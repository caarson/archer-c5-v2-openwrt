/* SPDX-License-Identifier: GPL-2.0-only */
/* RAM observer: cached state under the existing reorder_lock; no decisions.
 * A separate snapshot spinlock protects only our fixed-size scalar copies.
 * Getter never takes a reorder lock. No reverse lock dependency.
 */
#define PHASE3_REORDER_SLOTS 4
struct phase3_reorder_entry {
	u32 calls, decision[8], at, incoming, before, head, stored, size;
	u32 released, timer_pending, expires, flags;
	u32 jump_at, jump_incoming, jump_before, jump_head;
	u8 tid;
	bool used, jump_valid;
};
static bool phase3_reorder_diag;
module_param_named(phase3_reorder_diag, phase3_reorder_diag, bool, 0400);
MODULE_PARM_DESC(phase3_reorder_diag, "Bounded RAM RX path/reorder observer");
static struct phase3_reorder_entry phase3_reorder_entries[PHASE3_REORDER_SLOTS];
static DEFINE_SPINLOCK(phase3_reorder_lock);
static u32 phase3_reorder_slot_full;
static atomic_t phase3_rx_path[4], phase3_rx_early[PHASE3_RX_REASON_MAX + 1];

static bool phase3_reorder_scope(struct ieee80211_sub_if_data *sdata)
{
	static const u8 station[ETH_ALEN] = {0x62, 0xe3, 0x27, 0x3c, 0x51, 0xa8};

	return READ_ONCE(phase3_reorder_diag) &&
	       sdata->vif.type == NL80211_IFTYPE_STATION &&
	       ether_addr_equal(sdata->vif.addr, station);
}

static void phase3_reorder_note(struct ieee80211_sub_if_data *sdata,
			       struct tid_ampdu_rx *agg, u16 incoming,
			       u16 before, unsigned int decision,
			       u32 released, u32 flags)
{
	struct phase3_reorder_entry *e = NULL;
	unsigned long irqflags;
	unsigned int i;
	u16 advance;

	if (!phase3_reorder_scope(sdata) || decision >= 8)
		return;
	/* Caller still holds agg->reorder_lock. Never dereference its skb:
	 * the normal path may already have freed that skb before this hook.
	 */
	spin_lock_irqsave(&phase3_reorder_lock, irqflags);
	for (i = 0; i < PHASE3_REORDER_SLOTS; i++) {
		if (!phase3_reorder_entries[i].used ||
		    phase3_reorder_entries[i].tid == agg->tid) {
			e = &phase3_reorder_entries[i];
			break;
		}
	}
	if (!e) {
		phase3_reorder_slot_full++;
		goto out;
	}
	e->used = true;
	e->tid = agg->tid;
	e->calls++;
	e->decision[decision]++;
	e->at = (u32)jiffies;
	e->incoming = incoming;
	e->before = before;
	e->head = agg->head_seq_num;
	e->stored = agg->stored_mpdu_num;
	e->size = agg->buf_size;
	e->released = released;
	e->timer_pending = timer_pending(&agg->reorder_timer);
	e->expires = (u32)READ_ONCE(agg->reorder_timer.expires);
	e->flags = flags;
	advance = (agg->head_seq_num - before) & 4095;
	if (!e->jump_valid && advance > 128) {
		e->jump_valid = true;
		e->jump_at = (u32)jiffies;
		e->jump_incoming = incoming;
		e->jump_before = before;
		e->jump_head = agg->head_seq_num;
	}
 out:
	spin_unlock_irqrestore(&phase3_reorder_lock, irqflags);
}

static void phase3_rx_path_note(struct ieee80211_rx_data *rx,
			       unsigned int stage, unsigned int amount)
{
	if (stage < 4 && rx->sta && phase3_reorder_scope(rx->sdata))
		atomic_add(amount, &phase3_rx_path[stage]);
}

static void phase3_rx_early_note(struct ieee80211_rx_data *rx,
				ieee80211_rx_result result)
{
	u32 index;

	if (!rx->sta || !phase3_reorder_scope(rx->sdata) ||
	    result == RX_CONTINUE || result == RX_QUEUED)
		return;
	index = (__force u32)result - (__force u32)RX_DROP;
	if (index > PHASE3_RX_REASON_MAX)
		index = PHASE3_RX_REASON_MAX;
	atomic_inc(&phase3_rx_early[index]);
}

static int phase3_reorder_get(char *buffer, const struct kernel_param *unused)
{
	struct phase3_reorder_entry copy[PHASE3_REORDER_SLOTS];
	unsigned long irqflags;
	unsigned int i, j;
	u32 full;
	int n;

	spin_lock_irqsave(&phase3_reorder_lock, irqflags);
	memcpy(copy, phase3_reorder_entries, sizeof(copy));
	full = phase3_reorder_slot_full;
	spin_unlock_irqrestore(&phase3_reorder_lock, irqflags);
	n = scnprintf(buffer, PAGE_SIZE,
		      "reorder_schema=1 enabled=%u slots=4 slot_full=%u decisions=0:unmarked,1:prestart_old_direct,2:stale_drop,3:stored_dup_drop,4:inorder_direct,5:buffered\n",
		      READ_ONCE(phase3_reorder_diag), full);
	for (i = 0; i < PHASE3_REORDER_SLOTS; i++) {
		const struct phase3_reorder_entry *e = &copy[i];

		if (!e->used)
			continue;
		n += scnprintf(buffer + n, PAGE_SIZE - n,
			       "reorder_tid=%u calls=%u at=%u incoming=%u before=%u head=%u stored=%u size=%u released=%u pending=%u expires=%u flags=%08x decisions=",
			       e->tid, e->calls, e->at, e->incoming, e->before,
			       e->head, e->stored, e->size, e->released,
			       e->timer_pending, e->expires, e->flags);
		for (j = 0; j < 8; j++)
			n += scnprintf(buffer + n, PAGE_SIZE - n, "%s%u",
				       j ? "," : "", e->decision[j]);
		n += scnprintf(buffer + n, PAGE_SIZE - n,
			       " first_advance=%u,%u,%u,%u,%u\n",
			       e->jump_valid, e->jump_at, e->jump_incoming,
			       e->jump_before, e->jump_head);
	}
	return n;
}
static const struct kernel_param_ops phase3_reorder_ops = {
	.get = phase3_reorder_get,
};
module_param_cb(phase3_reorder_snapshot, &phase3_reorder_ops, NULL, 0400);

static int phase3_rx_path_get(char *buffer, const struct kernel_param *unused)
{
	unsigned int i;
	int n;

	n = scnprintf(buffer, PAGE_SIZE,
		      "rx_path_schema=1 enabled=%u invoke=%u checks_passed=%u reorder_return=%u released_frames=%u finite_trial_counters_modulo32=1\n",
		      READ_ONCE(phase3_reorder_diag),
		      (u32)atomic_read(&phase3_rx_path[0]),
		      (u32)atomic_read(&phase3_rx_path[1]),
		      (u32)atomic_read(&phase3_rx_path[2]),
		      (u32)atomic_read(&phase3_rx_path[3]));
	for (i = 0; i <= PHASE3_RX_REASON_MAX; i++) {
		u32 count = atomic_read(&phase3_rx_early[i]);

		if (count)
			n += scnprintf(buffer + n, PAGE_SIZE - n,
				       "early_reason=%u count=%u\n", i, count);
	}
	return n;
}
static const struct kernel_param_ops phase3_rx_path_ops = {
	.get = phase3_rx_path_get,
};
module_param_cb(phase3_rx_path_snapshot, &phase3_rx_path_ops, NULL, 0400);
