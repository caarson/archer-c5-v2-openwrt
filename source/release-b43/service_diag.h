/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_SERVICE_DIAG_H_
#define B43_SERVICE_DIAG_H_

#include <linux/types.h>
#include <linux/timekeeping.h>

/* Metadata only. Callers hold wl->mutex, except the single READ/WRITE_ONCE
 * hardirq_clock word. No ring/descriptor/MMIO access, allocations or policy.
 * Durations use modulo-2^32 units of 1024ns, not host timestamps.
 */
enum b43_service_id {
	B43_SERVICE_RX, B43_SERVICE_TX, B43_SERVICE_IP,
	B43_SERVICE_BH, B43_SERVICE_LOCK, B43_SERVICE_IRQ,
	B43_SERVICE_COUNT
};

struct b43_service_span {
	u32 last, max;
	unsigned long end;
};

#define B43_IRQ_HISTORY 8

struct b43_irq_event {
	u32 hard, lock, thread, rx_begin, rx_end, end, reason, dma0;
};

struct b43_service_diag {
	struct b43_service_span span[B43_SERVICE_COUNT];
	struct b43_service_span first_span[B43_SERVICE_COUNT];
	u32 hardirq_clock, handoff, max_handoff;
	u32 rx_gap, max_rx_gap, tx_entries, max_tx_entries;
	u32 first_pub_gap, first_rx_gap, first_handoff, first_tx_entries;
	u32 irqs_since_rx, rx_gap_irqs, first_rx_gap_irqs;
	unsigned long first_j;
	bool rx_seen, first_seen;
	struct b43_irq_event history[B43_IRQ_HISTORY];
	struct b43_irq_event frozen_history[B43_IRQ_HISTORY];
	struct b43_irq_event active_irq, frozen_active_irq;
	u32 history_next, history_count, frozen_next, frozen_count;
	u32 freeze_clock, freeze_rx_end;
	bool irq_active, history_frozen, frozen_irq_active;

};

/* IRQ-safe cached clock; low bits wrap after about 73 minutes. */
static inline u32 b43_service_clock(void)
{
	return (u32)(ktime_get_mono_fast_ns() >> 10);
}

static inline void b43_service_note(struct b43_service_diag *s,
				    enum b43_service_id id,
				    unsigned long begin, unsigned long end)
{
	struct b43_service_span *p;
	u32 elapsed = (u32)(end - begin);

	if ((unsigned int)id >= B43_SERVICE_COUNT)
		return;
	p = &s->span[id];
	p->last = elapsed;
	if (elapsed > p->max)
		p->max = elapsed;
	p->end = end;
	if (id == B43_SERVICE_RX) {
		s->rx_seen = true;
		if (s->irq_active)
			s->active_irq.rx_end = (u32)end;
	}
}

static inline void b43_service_rx_begin(struct b43_service_diag *s,
					unsigned long now)
{
	if (s->irq_active)
		s->active_irq.rx_begin = (u32)now;
	s->rx_gap_irqs = s->irqs_since_rx;
	s->irqs_since_rx = 0;
	s->rx_gap = s->rx_seen ?
		(u32)(now - s->span[B43_SERVICE_RX].end) : 0;
	if (s->rx_gap > s->max_rx_gap)
		s->max_rx_gap = s->rx_gap;
}

/* All history fields are protected by the existing wl->mutex.
 * The hard IRQ timestamp is copied from the existing READ_ONCE word.
 * No added MMIO, descriptor access, allocations, waits, or packet logging.
 */
static inline void b43_service_irq_begin(struct b43_service_diag *s,
					u32 hard, u32 lock, u32 thread,
					u32 reason, u32 dma0)
{
	s->active_irq = (struct b43_irq_event) {
		.hard = hard, .lock = lock, .thread = thread,
		.reason = reason, .dma0 = dma0,
	};
	s->irq_active = true;
}

static inline void b43_service_irq_end(struct b43_service_diag *s, u32 end)
{
	if (!s->irq_active)
		return;
	s->active_irq.end = end;
	s->history[s->history_next] = s->active_irq;
	s->history_next = (s->history_next + 1) % B43_IRQ_HISTORY;
	if (s->history_count < B43_IRQ_HISTORY)
		s->history_count++;
	s->irq_active = false;
}

/* Freeze BEFORE warning output and RX handling. The active event is
 * incomplete by definition; end=0 and rx_begin=0 must not mean a duration.
 */
static inline void b43_service_irq_freeze(struct b43_service_diag *s, u32 now)
{
	unsigned int i;

	if (s->history_frozen)
		return;
	s->freeze_clock = now;
	s->freeze_rx_end = (u32)s->span[B43_SERVICE_RX].end;
	s->frozen_next = s->history_next;
	s->frozen_count = s->history_count;
	for (i = 0; i < B43_IRQ_HISTORY; i++)
		s->frozen_history[i] = s->history[i];
	s->frozen_irq_active = s->irq_active;
	s->frozen_active_irq = s->irq_active ? s->active_irq :
		(struct b43_irq_event) { 0 };
	s->history_frozen = true;
}

static inline void b43_service_first(struct b43_service_diag *s,
				     unsigned long now, unsigned long pub_j)
{
	unsigned int i;

	if (s->first_seen)
		return;
	s->first_seen = true;
	s->first_j = now;
	s->first_pub_gap = (u32)(now - pub_j);
	s->first_rx_gap = s->rx_gap;
	s->first_handoff = s->handoff;
	s->first_rx_gap_irqs = s->rx_gap_irqs;
	s->first_tx_entries = s->tx_entries;
	for (i = 0; i < B43_SERVICE_COUNT; i++)
		s->first_span[i] = s->span[i];
}

#endif /* B43_SERVICE_DIAG_H_ */
