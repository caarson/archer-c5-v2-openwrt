/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_RX_BOUNDARY_DIAG_H
#define B43_RX_BOUNDARY_DIAG_H

/* Cached header metadata only. The DMA RX caller holds wl->mutex. */
enum b43_rxb_class {
	B43_RXB_SHORT, B43_RXB_NULL, B43_RXB_CIPHER, B43_RXB_BAD_LLC,
	B43_RXB_SNAP_OTHER, B43_RXB_ARP, B43_RXB_IPV6, B43_RXB_EAPOL,
	B43_RXB_IPV4_OTHER, B43_RXB_TEST, B43_RXB_CLASSES
};
#define B43_RXB_HISTORY 8
#define B43_RXB_FLOWS 8

struct b43_rxb_sample {
	u32 at, macstat, flags, tcp_seq;
	u16 length, fc, seqctl, dport, ip_length;
	u8 tid, class, amsdu, pn[6];
};
struct b43_rxb_flow {
	u32 packets, ip_bytes, tcp_seq, at;
	u16 dport, ip_length;
};
struct b43_rxb_diag {
	u32 hw_amsdu_seen, hw_amsdu_dropped;
	bool hw_amsdu_valid[2];
	struct b43_rxb_sample hw_amsdu_first[2];
	u32 classes[B43_RXB_CLASSES], next, count, slot_full;
	bool first_bad_valid;
	struct b43_rxb_sample first_bad, recent[B43_RXB_HISTORY];
	struct b43_rxb_flow flow[B43_RXB_FLOWS];
};

static inline u16 b43_rxb_be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
}

/* All accesses are bounded by linear skb bytes. No crypto, skb mutation,
 * allocation or MMIO. A-MSDU observation covers its FIRST subframe only.
 * Classification is evidence, never an accept/drop or recovery decision.
 */
static inline void b43_rxb_parse(struct b43_rxb_sample *s,
				const u8 *p, unsigned int n,
				unsigned int hdrlen, bool decrypted)
{
	unsigned int off, qos, total, ihl, tcp_len, available;
	const u8 *ip, *tcp;
	u16 proto;

	s->class = B43_RXB_SHORT;
	if (n < 24 || hdrlen < 24 || hdrlen > n)
		return;
	s->fc = p[0] | ((u16)p[1] << 8);
	s->seqctl = p[22] | ((u16)p[23] << 8);
	s->tid = 255;
	if (s->fc & 0x0040) {
		s->class = B43_RXB_NULL;
		return;
	}
	if (s->fc & 0x0080) {
		qos = (s->fc & 0x0300) == 0x0300 ? 30 : 24;
		if (qos + 2 > hdrlen)
			return;
		s->tid = p[qos] & 15;
		s->amsdu = !!(p[qos] & 0x80);
	}
	off = hdrlen;
	if (s->fc & 0x4000) {
		if (n - off < 8)
			return;
		s->pn[0] = p[off];
		s->pn[1] = p[off + 1];
		s->pn[2] = p[off + 4];
		s->pn[3] = p[off + 5];
		s->pn[4] = p[off + 6];
		s->pn[5] = p[off + 7];
		if (!decrypted) {
			s->class = B43_RXB_CIPHER;
			return;
		}
		off += 8;
	}
	available = n - off;
	if (s->amsdu) {
		if (available < 14)
			return;
		total = b43_rxb_be16(p + off + 12);
		off += 14;
		available -= 14;
		if (total > available)
			return;
		available = total;
	}
	if (available < 8)
		return;
	if (p[off] != 0xaa || p[off + 1] != 0xaa ||
	    p[off + 2] != 3 || p[off + 3] || p[off + 4] ||
	    (p[off + 5] != 0 && p[off + 5] != 0xf8)) {
		s->class = B43_RXB_BAD_LLC;
		return;
	}
	proto = b43_rxb_be16(p + off + 6);
	s->class = proto == 0x0806 ? B43_RXB_ARP :
		   proto == 0x86dd ? B43_RXB_IPV6 :
		   proto == 0x888e ? B43_RXB_EAPOL : B43_RXB_SNAP_OTHER;
	if (proto != 0x0800)
		return;
	s->class = B43_RXB_IPV4_OTHER;
	ip = p + off + 8;
	available -= 8;
	if (available < 20 || (ip[0] >> 4) != 4)
		return;
	ihl = (ip[0] & 15) * 4;
	total = b43_rxb_be16(ip + 2);
	if (ihl < 20 || ihl > available || total > available ||
	    total < ihl + 20 || ip[9] != 6 ||
	    (b43_rxb_be16(ip + 6) & 0x3fff))
		return;
	if (ip[12] != 192 || ip[13] != 168 || ip[14] != 0 ||
	    ip[15] != 134 || ip[16] != 192 || ip[17] != 168 ||
	    ip[18] != 0 || ip[19] != 146)
		return;
	tcp = ip + ihl;
	tcp_len = (tcp[12] >> 4) * 4;
	if (b43_rxb_be16(tcp) != 5209 || tcp_len < 20 ||
	    tcp_len > total - ihl)
		return;
	s->class = B43_RXB_TEST;
	s->dport = b43_rxb_be16(tcp + 2);
	s->tcp_seq = ((u32)tcp[4] << 24) | ((u32)tcp[5] << 16) |
		     ((u32)tcp[6] << 8) | tcp[7];
	s->ip_length = total;
}

static inline void b43_rxb_record(struct b43_rxb_diag *d,
				 const struct b43_rxb_sample *s)
{
	unsigned int i;

	if (s->class >= B43_RXB_CLASSES)
		return;
	d->classes[s->class]++;
	d->recent[d->next] = *s;
	d->next = (d->next + 1) % B43_RXB_HISTORY;
	if (d->count < B43_RXB_HISTORY)
		d->count++;
	if (!d->first_bad_valid && s->class == B43_RXB_BAD_LLC) {
		d->first_bad = *s;
		d->first_bad_valid = true;
	}
	if (s->class != B43_RXB_TEST)
		return;
	for (i = 0; i < B43_RXB_FLOWS; i++) {
		struct b43_rxb_flow *f = &d->flow[i];

		if (f->packets && f->dport != s->dport)
			continue;
		f->dport = s->dport;
		f->packets++;
		f->ip_bytes += s->ip_length;
		f->tcp_seq = s->tcp_seq;
		f->ip_length = s->ip_length;
		f->at = s->at;
		return;
	}
	d->slot_full++;
}
#endif
