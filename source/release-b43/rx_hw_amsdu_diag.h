/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Upstream247b2fc4 policy, scoped to AC and opt-in for matched RAM tests.
 * This hardware RX-status bit denotes an incomplete hardware deaggregation
 * record; it is distinct from the QoS A-MSDU bit of an on-air complete MPDU.
 */
static inline bool b43_hw_amsdu_should_drop(bool ac, bool enabled, u32 macstat)
{
	return ac && enabled && (macstat & B43_RX_MAC_AMSDU);
}

/* Existing wl->mutex protects these bounded counters/header snapshots.
 * Bytes are observed only for the same own-data scope as the boundary
 * observer. No allocation, MMIO, key lookup, payload storage or mutation.
 * PN bytes are header metadata, not a cryptographic-verification result.
 */
static inline void b43_hw_amsdu_observe(struct b43_rxb_diag *d, const u8 *p,
				      unsigned int n, u32 macstat, u32 at,
				      bool target)
{
	struct b43_rxb_sample sample = {
		.at = at,
		.macstat = macstat,
		.tid = 255,
		.class = B43_RXB_SHORT,
	};
	unsigned int offset, slot;
	u16 fc;

	if (!(macstat & B43_RX_MAC_AMSDU))
		return;
	d->hw_amsdu_seen++;
	if (!target)
		return;
	offset = sizeof(struct b43_plcp_hdr6) +
		 ((macstat & B43_RX_MAC_PADDING) ? 2 : 0);
	if (n < offset + 2)
		return;
	p += offset;
	n -= offset;
	fc = p[0] | ((u16)p[1] << 8);
	sample.length = min_t(unsigned int, n, 65535);
	b43_rxb_parse(&sample, p, n, ieee80211_hdrlen(cpu_to_le16(fc)),
		      !!(macstat & B43_RX_MAC_DEC));
	slot = sample.tid == 0 ? 0 : 1;
	if (!d->hw_amsdu_valid[slot]) {
		d->hw_amsdu_first[slot] = sample;
		d->hw_amsdu_valid[slot] = true;
	}
}
