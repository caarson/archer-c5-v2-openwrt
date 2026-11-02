/* SPDX-License-Identifier: GPL-2.0-only */
/* Observe the first TID0 MPDU that would shift the BA window by >128.
 * Called with the existing reorder_lock, before any possible skb free.
 * Only public 802.11 header fields and a bounded 8-byte IV are copied.
 * No acceptance, ordering, timer, crypto or delivery decisions are changed.
 */
struct phase3_reorder_trigger {
	u64 flags;
	u32 at, length;
	u16 incoming, before, predicted, fc, sc, hdrlen;
	u8 ra[ETH_ALEN], ta[ETH_ALEN], iv[8];
	bool valid, iv_present;
};
static struct phase3_reorder_trigger phase3_reorder_trigger;

static void phase3_reorder_trigger_capture(struct ieee80211_sub_if_data *sdata,
					  struct tid_ampdu_rx *agg,
					  struct sk_buff *skb, u16 incoming,
					  u16 before, u16 predicted)
{
	struct phase3_reorder_trigger t = {};
	struct ieee80211_hdr *hdr;
	unsigned long irqflags;

	if (!phase3_reorder_scope(sdata) || agg->tid ||
	    READ_ONCE(phase3_reorder_trigger.valid) || skb->len < 24)
		return;
	hdr = (void *)skb->data;
	t.flags = IEEE80211_SKB_RXCB(skb)->flag;
	t.at = (u32)jiffies;
	t.length = skb->len;
	t.incoming = incoming;
	t.before = before;
	t.predicted = predicted;
	t.fc = le16_to_cpu(hdr->frame_control);
	t.sc = le16_to_cpu(hdr->seq_ctrl);
	t.hdrlen = ieee80211_hdrlen(hdr->frame_control);
	ether_addr_copy(t.ra, hdr->addr1);
	ether_addr_copy(t.ta, hdr->addr2);
	if (ieee80211_has_protected(hdr->frame_control) &&
	    !(t.flags & RX_FLAG_IV_STRIPPED) &&
	    t.hdrlen >= 24 && t.hdrlen <= 36 &&
	    skb->len >= t.hdrlen + sizeof(t.iv) &&
	    !skb_copy_bits(skb, t.hdrlen, t.iv, sizeof(t.iv)))
		t.iv_present = true;
	t.valid = true;
	spin_lock_irqsave(&phase3_reorder_lock, irqflags);
	if (!phase3_reorder_trigger.valid)
		phase3_reorder_trigger = t;
	spin_unlock_irqrestore(&phase3_reorder_lock, irqflags);
}

/* Internal MAC-module bridge to the existing, validated PN boundary.
 * Exact SC/TID/IV match within one second. No skb or key pointer retained.
 * A match is correlation metadata, not an authentication decision.
 */
bool phase3_reorder_trigger_matches(struct ieee80211_sub_if_data *sdata,
				   unsigned int tid, u16 sc, const u8 *iv)
{
	const struct phase3_reorder_trigger *t = &phase3_reorder_trigger;
	unsigned long irqflags;
	bool match;

	if (!phase3_reorder_scope(sdata) || tid ||
	    !READ_ONCE(t->valid))
		return false;
	spin_lock_irqsave(&phase3_reorder_lock, irqflags);
	match = t->valid && t->iv_present && t->sc == sc &&
		(u32)((u32)jiffies - t->at) <= HZ &&
		!memcmp(t->iv, iv, sizeof(t->iv));
	spin_unlock_irqrestore(&phase3_reorder_lock, irqflags);
	return match;
}

static void phase3_reorder_trigger_hex(char *out, const u8 *in, unsigned int n)
{
	static const char hex[] = "0123456789abcdef";
	unsigned int i;

	for (i = 0; i < n; i++) {
		out[2 * i] = hex[in[i] >> 4];
		out[2 * i + 1] = hex[in[i] & 15];
	}
	out[2 * n] = 0;
}

static int phase3_reorder_trigger_get(char *buffer,
				     const struct kernel_param *unused)
{
	struct phase3_reorder_trigger t;
	unsigned long irqflags;
	char ra[13], ta[13], iv[17];

	spin_lock_irqsave(&phase3_reorder_lock, irqflags);
	t = phase3_reorder_trigger;
	spin_unlock_irqrestore(&phase3_reorder_lock, irqflags);
	phase3_reorder_trigger_hex(ra, t.ra, 6);
	phase3_reorder_trigger_hex(ta, t.ta, 6);
	phase3_reorder_trigger_hex(iv, t.iv, 8);
	return scnprintf(buffer, PAGE_SIZE,
			 "trigger_schema=1 enabled=%u valid=%u tid=0 at=%u incoming=%u before=%u predicted=%u fc=%04x sc=%04x length=%u hdr=%u flags=%016llx ra=%s ta=%s iv_present=%u iv=%s\n",
			 READ_ONCE(phase3_reorder_diag), t.valid, t.at,
			 t.incoming, t.before, t.predicted, t.fc, t.sc,
			 t.length, t.hdrlen, (unsigned long long)t.flags,
			 ra, ta, t.iv_present, iv);
}
static const struct kernel_param_ops phase3_reorder_trigger_ops = {
	.get = phase3_reorder_trigger_get,
};
module_param_cb(phase3_reorder_trigger_snapshot,
		&phase3_reorder_trigger_ops, NULL, 0400);
