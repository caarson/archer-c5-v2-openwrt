/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_DMA_COST_H_
#define B43_DMA_COST_H_
#include <linux/types.h>
/* All updates and snapshots are under the existing wl->mutex. No MMIO,
 * descriptors, allocations, packet bytes or policy decisions are examined.
 * Fixed systematic sample: every128th RX invocation, at most16384 packets.
 * Durations are elapsed nanoseconds (include preemption), modulo2^32.
 * Nested REPLACE includes ALLOC and MAP: never sum these overlapping spans.
 */
#define B43_DMA_COST_DIVISOR 128U
#define B43_DMA_COST_LIMIT 16384U
enum b43_dma_cost_id {
 B43_DMA_COST_HEADER, B43_DMA_COST_ALLOC, B43_DMA_COST_MAP,
 B43_DMA_COST_REPLACE, B43_DMA_COST_UNMAP, B43_DMA_COST_HANDOFF,
 B43_DMA_COST_CLOCK, B43_DMA_COST_COUNT
};
struct b43_dma_cost_span { u64 ns; u32 calls, max_ns; };
struct b43_dma_cost {
 u32 rx_calls, sampled;
 struct b43_dma_cost_span span[B43_DMA_COST_COUNT];
};
static inline bool b43_dma_cost_sample(struct b43_dma_cost *s)
{
 s->rx_calls++;
 if ((s->rx_calls & (B43_DMA_COST_DIVISOR - 1)) ||
     s->sampled >= B43_DMA_COST_LIMIT)
  return false;
 s->sampled++;
 return true;
}
static inline void b43_dma_cost_note(struct b43_dma_cost *s,
                                    unsigned int id, u32 begin, u32 end)
{
 struct b43_dma_cost_span *p;
 u32 elapsed = end - begin;
 if (id >= B43_DMA_COST_COUNT)
  return;
 p = &s->span[id];
 p->calls++;
 p->ns += elapsed;
 if (elapsed > p->max_ns)
  p->max_ns = elapsed;
}
#endif
