/* SPDX-License-Identifier: GPL-2.0-only */
/* Correlate one exact reorder trigger at the existing PN decision boundary.
 * Reason 0: existing PN-accepted path; 1: existing replay rejection;
 * 2: existing noncanonical-IV guard rejection (PN not decoded).
 * Key identity is metadata; key material and pointers are never retained.
 */
static struct phase3_pn_record phase3_trigger_pn;
static bool phase3_trigger_pn_valid, phase3_trigger_pn_present;
static DEFINE_SPINLOCK(phase3_trigger_pn_lock);

static void phase3_reorder_pn_observe(struct ieee80211_rx_data *rx,
				    const u8 *pn, const u8 *previous,
				    const u8 *iv, int hdrlen, u8 reason)
{
	struct phase3_pn_record t = {};
	struct ieee80211_hdr *hdr;
	unsigned long irqflags;
	int queue = rx->security_idx;

	if (!READ_ONCE(phase3_pn_diag) ||
	    READ_ONCE(phase3_trigger_pn_valid) || !rx->key ||
	    !(rx->key->conf.flags & IEEE80211_KEY_FLAG_PAIRWISE) ||
	    rx->sdata->vif.type != NL80211_IFTYPE_STATION ||
	    queue < 0 || queue > IEEE80211_NUM_TIDS ||
	    hdrlen < 24 || hdrlen > 36 || reason > 2 ||
	    rx->skb->len < (unsigned int)hdrlen + 8)
		return;
	hdr = (void *)rx->skb->data;
	if (!phase3_reorder_trigger_matches(rx->sdata, queue,
					   le16_to_cpu(hdr->seq_ctrl), iv))
		return;
	t.flags = IEEE80211_SKB_RXCB(rx->skb)->flag;
	t.observed_jiffies = (u32)jiffies;
	t.key_color = rx->key->color;
	t.cipher = rx->key->conf.cipher;
	t.length = rx->skb->len;
	t.sequence = le16_to_cpu(hdr->seq_ctrl);
	t.frame_control = le16_to_cpu(hdr->frame_control);
	t.header_length = hdrlen;
	t.queue = queue;
	t.key_index = rx->key->conf.keyidx;
	t.kind = reason;
	memcpy(t.iv, iv, sizeof(t.iv));
	if (pn && previous) {
		memcpy(t.pn, pn, sizeof(t.pn));
		memcpy(t.previous, previous, sizeof(t.previous));
	}
	spin_lock_irqsave(&phase3_trigger_pn_lock, irqflags);
	if (!phase3_trigger_pn_valid) {
		phase3_trigger_pn = t;
		phase3_trigger_pn_present = pn && previous;
		phase3_trigger_pn_valid = true;
	}
	spin_unlock_irqrestore(&phase3_trigger_pn_lock, irqflags);
}

static void phase3_trigger_pn_hex(char *out, const u8 *in, unsigned int n)
{
	static const char hex[] = "0123456789abcdef";
	unsigned int i;

	for (i = 0; i < n; i++) {
		out[2 * i] = hex[in[i] >> 4];
		out[2 * i + 1] = hex[in[i] & 15];
	}
	out[2 * n] = 0;
}

static int phase3_trigger_pn_get(char *buffer,
				const struct kernel_param *unused)
{
	struct phase3_pn_record t;
	unsigned long irqflags;
	bool valid, present;
	char pn[13], previous[13], iv[17];

	spin_lock_irqsave(&phase3_trigger_pn_lock, irqflags);
	t = phase3_trigger_pn;
	valid = phase3_trigger_pn_valid;
	present = phase3_trigger_pn_present;
	spin_unlock_irqrestore(&phase3_trigger_pn_lock, irqflags);
	phase3_trigger_pn_hex(pn, t.pn, 6);
	phase3_trigger_pn_hex(previous, t.previous, 6);
	phase3_trigger_pn_hex(iv, t.iv, 8);
	return scnprintf(buffer, PAGE_SIZE,
			 "pn_trigger_schema=1 enabled=%u valid=%u reason=%u j=%u key_color=%u cipher=%08x queue=%u key_index=%u flags=%016llx sequence=%04x fc=%04x length=%u hdr=%u pn_present=%u pn=%s previous=%s iv=%s\n",
			 READ_ONCE(phase3_pn_diag), valid, t.kind,
			 t.observed_jiffies, t.key_color, t.cipher, t.queue,
			 t.key_index, (unsigned long long)t.flags,
			 t.sequence, t.frame_control, t.length, t.header_length,
			 present, pn, previous, iv);
}
static const struct kernel_param_ops phase3_trigger_pn_ops = {
	.get = phase3_trigger_pn_get,
};
module_param_cb(phase3_reorder_pn_snapshot, &phase3_trigger_pn_ops, NULL, 0400);
