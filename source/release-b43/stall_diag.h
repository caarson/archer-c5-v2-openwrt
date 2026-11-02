/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_STALL_DIAG_H_
#define B43_STALL_DIAG_H_

#include <linux/atomic.h>
#include <linux/types.h>
#include "service_diag.h"
#include "dma_cost.h"
#include "rx_pool_diag.h"

enum b43_stall_counter {
	B43_SD_TX_POST,
	B43_SD_TX_COMPLETE,
	B43_SD_TX_STATUS,
	B43_SD_TX_INTERMEDIATE,
	B43_SD_TX_AMPDU_STATUS,
	B43_SD_TX_AMPDU_MPDUS,
	B43_SD_TX_AMPDU_ACKED,
	B43_SD_TX_BAD_COOKIE,
	B43_SD_TX_OUT_OF_ORDER,
	B43_SD_TX_SKIP,
	B43_SD_TX_POISON,
	B43_SD_TX_INVALID_BOUNDS,
	B43_SD_TX_MAP_FAIL,
	B43_SD_QUEUE_STOP,
	B43_SD_QUEUE_WAKE,
	B43_SD_RX_BUFFER,
	B43_SD_RX_ZERO,
	B43_SD_RX_POISON,
	B43_SD_RX_ALLOC_FAIL,
	B43_SD_RX_OVERSIZE,
	B43_SD_RX_UNDERRUN,
	B43_SD_IRQ_THREAD,
	B43_SD_BA_RX_START,
	B43_SD_BA_RX_STOP,
	B43_SD_BA_TX_OPERATIONAL,
	B43_SD_BA_TX_STOP,
	B43_SD_TX_HW_KEY,
	B43_SD_RX_HW_DECRYPTED,
	B43_SD_TX_SECOND_INVALID,
	B43_SD_RX_HANDOFF,
	B43_SD_RX_DROP,
	B43_SD_COUNT
};

/* All fields below are protected by the existing wl->mutex.
 * Counters are modulo2^32. pub_arg is the driver's argument, not a readback.
 * Cursor indices are actual implementation slots, never invented generations.
 */
#include "rx_boundary_diag.h"

struct b43_rx_flow_diag {
 u32 beacon, own_data, group_data, other_data;
 u32 own_dec_error_attempt, own_fcs, own_hw_decrypted, own_handoff;
 u32 pub_count;
 int pub_arg;
 unsigned long pub_j;
 bool underrun_pending, first_captured, post_valid;
 u32 reason;
 unsigned long pre_j, post_j;
 int pre_driver_next, pre_hw_next, pre_nr, pre_pub_arg;
 u32 pre_pub_count, pre_rx_buffer, pre_handoff;
 int post_driver_next, post_calls, post_pub_arg;
 u32 post_pub_count, post_rx_buffer, post_handoff;
};

/* Private driver state only; no external kernel/mac80211 struct changes. */
struct b43_stall_diag {
	struct b43_rxb_diag rx_boundary;
	struct b43_rx_flow_diag rx_flow;
	struct b43_service_diag service;
	struct b43_dma_cost dma_cost;
	struct b43_rx_pool_diag rx_pool;
	atomic_t count[B43_SD_COUNT]; /* modulo 2^32, never reset by a read */
	atomic_t ba_win[16]; /* last TX BA callback per TID, not every peer */
	u32 ampdu_hist[5]; /* 0/1, 2-7, 8-15, 16-31, >=32 MPDUs/status */
	u32 first_bad_status[8]; /* first absent second-package valid bit */
	u32 last_status[8]; /* copied from normal FIFO read; never reread FIFO */
	unsigned long posted[5], reclaimed[5];
	u32 rx_batch_max;
	u32 first_ownership_reason, first_ownership_raw[8];
	int first_ownership_ring, first_ownership_nr, first_ownership_used;
	int first_ownership_producer, first_ownership_slot;
	u32 first_ownership_mpdus, first_ownership_post, first_ownership_complete;
	unsigned long first_ownership_j;
	bool registered;
};

extern bool b43_stall_diag_enabled;
extern bool b43_stall_hot_detail;
extern bool b43_dma_cost_enabled;
extern bool b43_rx_bidir;
struct b43_wldev;
int b43_stall_diag_register(struct b43_wldev *dev);
void b43_stall_diag_unregister(struct b43_wldev *dev);

#define B43_SD_INC(dev, name) do { \
	if (unlikely(b43_stall_diag_enabled)) \
		atomic_inc(&(dev)->stall_diag.count[B43_SD_ ## name]); \
} while (0)
#define B43_SD_ADD(dev, name, n) do { \
	if (unlikely(b43_stall_diag_enabled)) \
		atomic_add((n), &(dev)->stall_diag.count[B43_SD_ ## name]); \
} while (0)

static inline unsigned int b43_stall_diag_bucket(unsigned int n)
{
	return n < 2 ? 0 : n < 8 ? 1 : n < 16 ? 2 : n < 32 ? 3 : 4;
}

/* Observations only: reasons 1=geometry, 2=alignment, 4=budget.
 * No descriptor dereference, hardware access, allocation or policy decision.
 */
static inline unsigned int b43_sd_ownership_reason(int nr, int used,
					 int producer, int slot,
					 unsigned int mpdus)
{
	unsigned int frames = mpdus ? mpdus : 1;
	unsigned int skipped = 0;
	int first, next;

	if (nr <= 0 || nr > 256 || (nr & 1) || used < 2 || used > nr ||
	    producer < 0 || producer >= nr || slot < 0 || slot >= nr)
		return 1;
	first = producer - used + 1;
	if (first < 0)
		first += nr;
	if ((used & 1) || (first & 1) || (slot & 1))
		return 2;
	next = first + 2 == nr ? 0 : first + 2;
	if (slot != first && slot == next)
		skipped = 1;
	return frames > (unsigned int)used / 2 - skipped ? 4 : 0;
}

#endif /* B43_STALL_DIAG_H_ */
