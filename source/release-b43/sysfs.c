// SPDX-License-Identifier: GPL-2.0-or-later
/*

  Broadcom B43 wireless driver

  SYSFS support routines

  Copyright (c) 2006 Michael Buesch <m@bues.ch>


*/

#include <linux/capability.h>
#include <linux/io.h>

#include "b43.h"
#include "sysfs.h"
#include "main.h"
#include "phy_common.h"
#include "phy_ac.h"
#include "dma.h"
#include <linux/module.h>
#include <net/page_pool/helpers.h>
#include <linux/jiffies.h>

#define GENERIC_FILESIZE	64

static int get_integer(const char *buf, size_t count)
{
	char tmp[10 + 1] = { 0 };
	int ret = -EINVAL;

	if (count == 0)
		goto out;
	count = min_t(size_t, count, 10);
	memcpy(tmp, buf, count);
	ret = simple_strtol(tmp, NULL, 10);
      out:
	return ret;
}

static ssize_t b43_attr_interfmode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct b43_wldev *wldev = dev_to_b43_wldev(dev);
	ssize_t count = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	mutex_lock(&wldev->wl->mutex);

	if (wldev->phy.type != B43_PHYTYPE_G) {
		mutex_unlock(&wldev->wl->mutex);
		return -ENOSYS;
	}

	switch (wldev->phy.g->interfmode) {
	case B43_INTERFMODE_NONE:
		count = sysfs_emit(buf, "0 (No Interference Mitigation)\n");
		break;
	case B43_INTERFMODE_NONWLAN:
		count = sysfs_emit(buf,
				   "1 (Non-WLAN Interference Mitigation)\n");
		break;
	case B43_INTERFMODE_MANUALWLAN:
		count = sysfs_emit(buf, "2 (WLAN Interference Mitigation)\n");
		break;
	default:
		B43_WARN_ON(1);
	}

	mutex_unlock(&wldev->wl->mutex);

	return count;
}

static ssize_t b43_attr_interfmode_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct b43_wldev *wldev = dev_to_b43_wldev(dev);
	int err;
	int mode;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	mode = get_integer(buf, count);
	switch (mode) {
	case 0:
		mode = B43_INTERFMODE_NONE;
		break;
	case 1:
		mode = B43_INTERFMODE_NONWLAN;
		break;
	case 2:
		mode = B43_INTERFMODE_MANUALWLAN;
		break;
	case 3:
		mode = B43_INTERFMODE_AUTOWLAN;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&wldev->wl->mutex);

	if (wldev->phy.ops->interf_mitigation) {
		err = wldev->phy.ops->interf_mitigation(wldev, mode);
		if (err) {
			b43err(wldev->wl, "Interference Mitigation not "
			       "supported by device\n");
		}
	} else
		err = -ENOSYS;

	mutex_unlock(&wldev->wl->mutex);

	return err ? err : count;
}

static DEVICE_ATTR(interference, 0644,
		   b43_attr_interfmode_show, b43_attr_interfmode_store);

int b43_sysfs_register(struct b43_wldev *wldev)
{
	struct device *dev = wldev->dev->dev;

	B43_WARN_ON(b43_status(wldev) != B43_STAT_INITIALIZED);

	return device_create_file(dev, &dev_attr_interference);
}

void b43_sysfs_unregister(struct b43_wldev *wldev)
{
	struct device *dev = wldev->dev->dev;

	device_remove_file(dev, &dev_attr_interference);
}

/* Phase 2 lab diagnostics: disabled by default; no reset or repair action. */
bool b43_stall_diag_enabled;
bool b43_dma_cost_enabled;
module_param_named(dma_cost, b43_dma_cost_enabled, bool, 0400);
MODULE_PARM_DESC(dma_cost, "Sample bounded RX DMA operation durations (lab only, default off)");
/* Preserve all fault counters; optionally omit hot normal-RX detail. */
bool b43_stall_hot_detail = true;
module_param_named(stall_hot_detail, b43_stall_hot_detail, bool, 0400);
MODULE_PARM_DESC(stall_hot_detail, "Collect normal RX boundary and fine service timing (default on)");
module_param_named(stall_diag, b43_stall_diag_enabled, bool, 0400);
MODULE_PARM_DESC(stall_diag, "Enable read-only stall counters at module load (lab only)");

static const char * const b43_sd_names[] = {
	[B43_SD_RX_HANDOFF] = "rx_handoff",
	[B43_SD_RX_DROP] = "rx_drop",
	[B43_SD_TX_SECOND_INVALID] = "tx_second_invalid",
	[B43_SD_TX_POST] = "tx_post",
	[B43_SD_TX_COMPLETE] = "tx_complete",
	[B43_SD_TX_STATUS] = "tx_status",
	[B43_SD_TX_INTERMEDIATE] = "tx_intermediate",
	[B43_SD_TX_AMPDU_STATUS] = "tx_ampdu_status",
	[B43_SD_TX_AMPDU_MPDUS] = "tx_ampdu_mpdus",
	[B43_SD_TX_AMPDU_ACKED] = "tx_ampdu_acked",
	[B43_SD_TX_BAD_COOKIE] = "tx_bad_cookie",
	[B43_SD_TX_OUT_OF_ORDER] = "tx_out_of_order",
	[B43_SD_TX_SKIP] = "tx_skip",
	[B43_SD_TX_POISON] = "tx_poison",
	[B43_SD_TX_INVALID_BOUNDS] = "tx_invalid_bounds",
	[B43_SD_TX_MAP_FAIL] = "tx_map_fail",
	[B43_SD_QUEUE_STOP] = "queue_stop",
	[B43_SD_QUEUE_WAKE] = "queue_wake",
	[B43_SD_RX_BUFFER] = "rx_buffer",
	[B43_SD_RX_ZERO] = "rx_zero",
	[B43_SD_RX_POISON] = "rx_poison",
	[B43_SD_RX_ALLOC_FAIL] = "rx_alloc_fail",
	[B43_SD_RX_OVERSIZE] = "rx_oversize",
	[B43_SD_RX_UNDERRUN] = "rx_underrun",
	[B43_SD_IRQ_THREAD] = "irq_thread",
	[B43_SD_BA_RX_START] = "ba_rx_start",
	[B43_SD_BA_RX_STOP] = "ba_rx_stop",
	[B43_SD_BA_TX_OPERATIONAL] = "ba_tx_operational",
	[B43_SD_BA_TX_STOP] = "ba_tx_stop",
	[B43_SD_TX_HW_KEY] = "tx_hw_key",
	[B43_SD_RX_HW_DECRYPTED] = "rx_hw_decrypted",
};

/* The legacy dev_to_b43_wldev() assumes an SSB device wrapper.
 * This attribute also belongs to BCMA cores, whose device embedding differs.
 * Select the bus before converting; never interpret one layout as the other.
 */
static struct b43_wldev *b43_stall_diag_wldev(struct device *device)
{
	if (!device || !device->bus || !device->bus->name)
		return NULL;
#ifdef CPTCFG_B43_BCMA
	if (!strcmp(device->bus->name, "bcma"))
		return bcma_get_drvdata(container_of(device,
						    struct bcma_device, dev));
#endif
#ifdef CPTCFG_B43_SSB
	if (!strcmp(device->bus->name, "ssb")) {
		struct ssb_device *sdev = dev_to_ssb_dev(device);

		return sdev ? ssb_get_drvdata(sdev) : NULL;
	}
#endif
	return NULL;
}

static ssize_t stall_diag_show(struct device *device,
			       struct device_attribute *attr, char *buf)
{
	struct b43_wldev *dev;
	struct b43_stall_diag *d;
	struct b43_dmaring *rings[6];
	ssize_t n = 0;
	unsigned int i, j;
	bool live;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	dev = b43_stall_diag_wldev(device);
	if (!dev || !dev->wl || !dev->dev || dev->dev->dev != device)
		return -ENODEV;
	d = &dev->stall_diag;
	/* Never wait behind a potentially stuck interrupt/work handler. */
	if (!mutex_trylock(&dev->wl->mutex))
		return -EBUSY;
	live = b43_status(dev) == B43_STAT_STARTED;
	n += sysfs_emit_at(buf, n,
		"schema=2 enabled=%u live=%u jiffies=%lu status=%d mac_suspended=%d cac=%u\n",
		b43_stall_diag_enabled, live, jiffies, b43_status(dev),
		dev->mac_suspended, dev->cac_pending);
	n += sysfs_emit_at(buf, n, "hot_detail=%u\n", b43_stall_hot_detail);
	for (i = 0; i < B43_SD_COUNT; i++)
		n += sysfs_emit_at(buf, n, "%s=%u\n", b43_sd_names[i],
				   (u32)atomic_read(&d->count[i]));
	n += sysfs_emit_at(buf, n,
		"hist_0_1_2_7_8_15_16_31_32plus=%u,%u,%u,%u,%u rx_batch_max=%u\n",
		 d->ampdu_hist[0], d->ampdu_hist[1], d->ampdu_hist[2],
		 d->ampdu_hist[3], d->ampdu_hist[4], d->rx_batch_max);
	n += sysfs_emit_at(buf, n, "last_status=");
	for (i = 0; i < 8; i++)
		n += sysfs_emit_at(buf, n, "%08x%c", d->last_status[i],
				   i == 7 ? '\n' : ',');
	n += sysfs_emit_at(buf, n, "first_bad_status=");
	for (i = 0; i < 8; i++)
		n += sysfs_emit_at(buf, n, "%08x%c", d->first_bad_status[i],
				   i == 7 ? '\n' : ',');
	n += sysfs_emit_at(buf, n, "last_tx_ba_window_tid=");
	for (i = 0; i < 16; i++)
		n += sysfs_emit_at(buf, n, "%u%c",
			 (u32)atomic_read(&d->ba_win[i]), i == 15 ? '\n' : ',');
	n += sysfs_emit_at(buf, n,
		"first_ownership reason=%u j=%lu ring=%d nr=%d used=%d producer=%d slot=%d mpdus=%u post=%u complete=%u raw=",
		d->first_ownership_reason, d->first_ownership_j,
		d->first_ownership_ring, d->first_ownership_nr,
		d->first_ownership_used, d->first_ownership_producer,
		d->first_ownership_slot, d->first_ownership_mpdus,
		d->first_ownership_post, d->first_ownership_complete);
	for (i = 0; i < 8; i++)
		n += sysfs_emit_at(buf, n, "%08x%c", d->first_ownership_raw[i],
				   i == 7 ? '\n' : ',');
	{
		const struct b43_rx_flow_diag *r = &d->rx_flow;

		n += sysfs_emit_at(buf, n,
			"rxflow_schema=1 own_mac=%pM beacon=%u own_data=%u group_data=%u other_data=%u own_dec_error_attempt=%u own_fcs=%u own_hw_decrypted=%u own_handoff=%u\n",
			dev->wl->mac_addr, r->beacon, r->own_data, r->group_data,
			r->other_data, r->own_dec_error_attempt, r->own_fcs,
			r->own_hw_decrypted, r->own_handoff);
		n += sysfs_emit_at(buf, n,
			"rx_publish_argument=%d count=%u j=%lu\n",
			r->pub_arg, r->pub_count, r->pub_j);
		n += sysfs_emit_at(buf, n,
			"first_rx_underrun captured=%u post_valid=%u reason=%08x pre_j=%lu pre_driver_next=%d pre_hw_next=%d nr=%d pre_pub_arg=%d pre_pub_count=%u pre_rx_buffer=%u pre_handoff=%u post_j=%lu post_driver_next=%d loop_calls=%d post_pub_arg=%d post_pub_count=%u post_rx_buffer=%u post_handoff=%u\n",
			r->first_captured, r->post_valid, r->reason,
			r->pre_j, r->pre_driver_next, r->pre_hw_next, r->pre_nr,
			r->pre_pub_arg, r->pre_pub_count, r->pre_rx_buffer,
			r->pre_handoff, r->post_j, r->post_driver_next,
			r->post_calls, r->post_pub_arg, r->post_pub_count,
			r->post_rx_buffer, r->post_handoff);
	}
	{
		const struct b43_service_diag *s = &d->service;
		const struct b43_service_span *p = s->first_seen ?
			s->first_span : s->span;
		unsigned long at = s->first_seen ? s->first_j : jiffies;
		u32 pg = s->first_seen ? s->first_pub_gap :
			d->rx_flow.pub_count ? (u32)(at - d->rx_flow.pub_j) : 0;
		u32 gap = s->first_seen ? s->first_rx_gap : s->rx_gap;
		u32 hand = s->first_seen ? s->first_handoff : s->handoff;
		u32 seen = s->first_seen ?
			s->first_rx_gap_irqs : s->rx_gap_irqs;
		u32 entries = s->first_seen ?
			s->first_tx_entries : s->tx_entries;

		/* One view only: before an underrun show live spans; afterwards
		 * preserve the immutable pre-underrun spans. Tuple order is
		 * last and maximum duration, last-end only for RX/IRQ. Durations
		 * use 1024ns ticks; at/pg_j retain jiffies. n=TX entries/RX-gap IRQs.
		 */
		n += sysfs_emit_at(buf, n,
			"service_schema=2 view=%s tick_ns=1024 at=%lu pg_j=%u gap=%u hand=%u tx=%u,%u rx=%u,%u,%lu ip=%u,%u bh=%u,%u lock=%u,%u irq=%u,%u,%lu n=%u/%u\n",
			s->first_seen ? "first" : "live",
			at, pg, gap, hand,
			p[1].last, p[1].max,
			p[0].last, p[0].max, p[0].end,
			p[2].last, p[2].max,
			p[3].last, p[3].max,
			p[4].last, p[4].max,
			p[5].last, p[5].max, p[5].end, entries, seen);
	}
	if (!live || dev->phy.type != B43_PHYTYPE_AC ||
	    b43_using_pio_transfers(dev))
		goto out;
	/* No TX-status FIFO reads: those consume completion records. */
	n += sysfs_emit_at(buf, n,
		"irq_last=%08x irq_mask_sw=%08x irq_mask_hw=%08x macctl=%08x maccmd=%08x phy_state=%04x\n",
		READ_ONCE(dev->irq_reason), READ_ONCE(dev->irq_mask),
		b43_read32(dev, B43_MMIO_GEN_IRQ_MASK),
		b43_read32(dev, B43_MMIO_MACCTL),
		b43_read32(dev, B43_MMIO_MACCMD), dev->phy.ac->status_mask);
	rings[0] = dev->dma.tx_ring_AC_BK;
	rings[1] = dev->dma.tx_ring_AC_BE;
	rings[2] = dev->dma.tx_ring_AC_VI;
	rings[3] = dev->dma.tx_ring_AC_VO;
	rings[4] = dev->dma.tx_ring_mcast;
	rings[5] = dev->dma.rx_ring;
	for (i = 0; i < ARRAY_SIZE(rings); i++) {
		struct b43_dmaring *r = rings[i];
		u16 ctl, index, status;

		if (!r)
			continue;
		if (r->type == B43_DMA_64BIT) {
			ctl = r->tx ? B43_DMA64_TXCTL : B43_DMA64_RXCTL;
			index = r->tx ? B43_DMA64_TXINDEX : B43_DMA64_RXINDEX;
			status = r->tx ? B43_DMA64_TXSTATUS : B43_DMA64_RXSTATUS;
		} else {
			ctl = r->tx ? B43_DMA32_TXCTL : B43_DMA32_RXCTL;
			index = r->tx ? B43_DMA32_TXINDEX : B43_DMA32_RXINDEX;
			status = r->tx ? B43_DMA32_TXSTATUS : B43_DMA32_RXSTATUS;
		}
		n += sysfs_emit_at(buf, n,
			"%s%u used=%d nr=%d swslot=%d stopped=%u ctl=%08x index=%08x status=%08x",
			r->tx ? "tx" : "rx", r->index, r->used_slots,
			r->nr_slots, r->current_slot, r->stopped,
			b43_dma_read(r, ctl), b43_dma_read(r, index),
			b43_dma_read(r, status));
		if (r->type == B43_DMA_64BIT)
			n += sysfs_emit_at(buf, n, " error=%08x",
				b43_dma_read(r, r->tx ? B43_DMA64_TXERROR :
						       B43_DMA64_RXERROR));
		if (r->tx && r->index < 5)
			n += sysfs_emit_at(buf, n, " posted_j=%lu reclaimed_j=%lu",
				 d->posted[r->index], d->reclaimed[r->index]);
		n += sysfs_emit_at(buf, n, "\n");
	}
	for (j = 0; j < B43_QOS_QUEUE_NUM; j++)
		n += sysfs_emit_at(buf, n, "swq%u len=%u stopped=%u\n", j,
			skb_queue_len(&dev->wl->tx_queue[j]),
			READ_ONCE(dev->wl->tx_queue_stopped[j]));
out:
	mutex_unlock(&dev->wl->mutex);
	return n;
}
static DEVICE_ATTR_RO(stall_diag);

/* Separate bounded page: do not consume the existing ring snapshot budget.
 * These are configuration-register reads, never interrupt-reason ACKs or
 * TX-status FIFO reads. wl->mutex protects device state and cached counters.
 */
static ssize_t stall_irqcfg_show(struct device *device,
				 struct device_attribute *attr, char *buf)
{
	struct b43_wldev *dev;
	const struct b43_service_diag *s;
	u32 lazy0 = 0, lazy3 = 0, mask0 = 0;
	const struct b43_irq_event *events, *e;
	u32 next, count;
	unsigned int i, slot;
	ssize_t n;
	bool live;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	dev = b43_stall_diag_wldev(device);
	if (!dev || !dev->wl || !dev->dev || dev->dev->dev != device)
		return -ENODEV;
	if (!mutex_trylock(&dev->wl->mutex))
		return -EBUSY;
	s = &dev->stall_diag.service;
	live = b43_status(dev) == B43_STAT_STARTED &&
	       dev->phy.type == B43_PHYTYPE_AC &&
	       !b43_using_pio_transfers(dev);
	if (live) {
		/* D11 receive-lazy controls: also written in b43_chip_init(). */
		lazy0 = b43_read32(dev, 0x0100);
		lazy3 = b43_read32(dev, 0x010C);
		mask0 = b43_read32(dev, B43_MMIO_DMA0_IRQ_MASK);
	}
	n = sysfs_emit(buf,
		       "irqcfg_schema=1 enabled=%u live=%u jiffies=%lu lazy0=%08x lazy3=%08x dma0_mask=%08x hand=%u max_hand=%u tick_ns=1024 irq_saved=%08x dma0_saved=%08x max_gap=%u max_tx_entries=%u\n",
		       b43_stall_diag_enabled, live, jiffies, lazy0, lazy3, mask0,
		       s->handoff, s->max_handoff, READ_ONCE(dev->irq_reason),
		       READ_ONCE(dev->dma_reason[0]), s->max_rx_gap, s->max_tx_entries);
	events = s->history_frozen ? s->frozen_history : s->history;
	next = s->history_frozen ? s->frozen_next : s->history_next;
	count = s->history_frozen ? s->frozen_count : s->history_count;
	count = min_t(u32, count, B43_IRQ_HISTORY);
	n += sysfs_emit_at(buf, n,
			   "irq_history_schema=1 frozen=%u count=%u capacity=8 tick_ns=1024 freeze=%u last_rx_end=%u\\n",
			   s->history_frozen, count,
			   s->freeze_clock, s->freeze_rx_end);
	for (i = 0; i < count; i++) {
		slot = (next + B43_IRQ_HISTORY - count + i) %
		       B43_IRQ_HISTORY;
		e = &events[slot];
		n += sysfs_emit_at(buf, n,
				   "irq_history=%u hard=%u lock=%u thread=%u rx_begin=%u rx_end=%u end=%u reason=%08x dma0=%08x\\n",
				   i, e->hard, e->lock, e->thread,
				   e->rx_begin, e->rx_end, e->end,
				   e->reason, e->dma0);
	}
	if (s->history_frozen && s->frozen_irq_active) {
		e = &s->frozen_active_irq;
		n += sysfs_emit_at(buf, n,
				   "irq_history_active=1 hard=%u lock=%u thread=%u rx_begin=%u rx_end=%u end=%u reason=%08x dma0=%08x\\n",
				   e->hard, e->lock, e->thread,
				   e->rx_begin, e->rx_end, e->end,
				   e->reason, e->dma0);
	}
	mutex_unlock(&dev->wl->mutex);
	return n;
}
static DEVICE_ATTR_RO(stall_irqcfg);


static ssize_t b43_rxb_format(char *buf, const struct b43_rxb_diag *d)
{
	ssize_t n;
	unsigned int i, slot;
	const struct b43_rxb_sample *s;

	n = sysfs_emit(buf,
		       "rx_boundary_schema=1 class_count=%u history_count=%u capacity=8 first_msdu_only=1 slot_full=%u\n",
		       B43_RXB_CLASSES, d->count, d->slot_full);
	for (i = 0; i < B43_RXB_CLASSES; i++)
		n += sysfs_emit_at(buf, n, "rxb_class=%u count=%u\n",
				   i, d->classes[i]);
	for (i = 0; i < B43_RXB_FLOWS; i++) {
		const struct b43_rxb_flow *f = &d->flow[i];

		if (f->packets)
			n += sysfs_emit_at(buf, n,
					   "rxb_flow=%u dport=%u packets=%u ip_bytes=%u seq=%08x ip_len=%u at=%u\n",
					   i, f->dport, f->packets, f->ip_bytes,
					   f->tcp_seq, f->ip_length, f->at);
	}
	n += sysfs_emit_at(buf, n,
			   "hw_amsdu_schema=1 enabled=%u seen=%u dropped=%u tid0_valid=%u other_valid=%u\n",
			   b43_rx_hw_amsdu_guard, d->hw_amsdu_seen,
			   d->hw_amsdu_dropped, d->hw_amsdu_valid[0],
			   d->hw_amsdu_valid[1]);
	for (i = 0; i < 2; i++) {
		if (!d->hw_amsdu_valid[i])
			continue;
		s = &d->hw_amsdu_first[i];
		n += sysfs_emit_at(buf, n,
				   "hw_amsdu_sample=%u at=%u len=%u fc=%04x seqctl=%04x tid=%u class=%u amsdu=%u mac=%08x pn=%02x%02x%02x%02x%02x%02x\n",
				   i, s->at, s->length, s->fc, s->seqctl,
				   s->tid, s->class, s->amsdu, s->macstat,
				   s->pn[5], s->pn[4], s->pn[3], s->pn[2],
				   s->pn[1], s->pn[0]);
	}
	/* Emit the immutable first unexpected LLC, then the bounded tail. */
	for (i = 0; i <= d->count; i++) {
		if (!i) {
			if (!d->first_bad_valid)
				continue;
			s = &d->first_bad;
		} else {
			slot = (d->next + B43_RXB_HISTORY - d->count + i - 1) %
			       B43_RXB_HISTORY;
			s = &d->recent[slot];
		}
		n += sysfs_emit_at(buf, n,
				   "rxb_sample=%u at=%u len=%u fc=%04x seqctl=%04x tid=%u class=%u amsdu=%u mac=%08x flags_low32=%08x pn=%02x%02x%02x%02x%02x%02x port=%u tcpseq=%08x iplen=%u\n",
				   i, s->at, s->length, s->fc, s->seqctl,
				   s->tid, s->class, s->amsdu, s->macstat,
				   s->flags, s->pn[5], s->pn[4], s->pn[3],
				   s->pn[2], s->pn[1], s->pn[0], s->dport,
				   s->tcp_seq, s->ip_length);
	}
	return n;
}

static ssize_t stall_rxboundary_show(struct device *device,
				     struct device_attribute *attr, char *buf)
{
	struct b43_wldev *dev;
	ssize_t n;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	dev = b43_stall_diag_wldev(device);
	if (!dev || !dev->wl || !dev->dev || dev->dev->dev != device)
		return -ENODEV;
	if (!mutex_trylock(&dev->wl->mutex))
		return -EBUSY;
	n = b43_rxb_format(buf, &dev->stall_diag.rx_boundary);
	mutex_unlock(&dev->wl->mutex);
	return n;
}
static DEVICE_ATTR_RO(stall_rxboundary);

static ssize_t stall_defer_show(struct device *device,
			       struct device_attribute *attr, char *buf)
{
	struct b43_wldev *dev;
	struct b43_wl *wl;
	ssize_t n;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	dev = b43_stall_diag_wldev(device);
	if (!dev || !dev->wl || !dev->dev || dev->dev->dev != device)
		return -ENODEV;
	wl = dev->wl;
	if (!mutex_trylock(&wl->mutex))
		return -EBUSY;
	/* Individual scalar reads, not an atomic queue/counter snapshot:
	 * delivery is deliberately outside this mutex.
	 */
	n = sysfs_emit(buf,
		       "defer_schema=1 enabled=%u rx_queued=%u rx_delivered=%u tx_queued=%u tx_delivered=%u rx_pending=%u tx_pending=%u rx_peak=%u tx_peak=%u last_ticks=%u max_ticks=%u tick_ns=1024 sync_mode=%u sync_headers=%u sync_payloads=%u sync_skips=%u sync_lastlen=%u\n",
		       b43_defer_mac, atomic_read(&wl->defer_rx_queued),
		       atomic_read(&wl->defer_rx_delivered),
		       atomic_read(&wl->defer_tx_queued),
		       atomic_read(&wl->defer_tx_delivered),
		       skb_queue_len(&wl->defer_rx_queue),
		       skb_queue_len(&wl->defer_txstatus_queue),
		       READ_ONCE(wl->defer_rx_peak), READ_ONCE(wl->defer_tx_peak),
		       READ_ONCE(wl->defer_last_ticks),
		       READ_ONCE(wl->defer_max_ticks), b43_rx_sync_len,
		       atomic_read(&wl->rxsync_headers),
		       atomic_read(&wl->rxsync_payloads),
		       atomic_read(&wl->rxsync_skips),
		       READ_ONCE(wl->rxsync_lastlen));
	n += sysfs_emit_at(buf, n, "defer_batch_schema=1 enabled=%u rx_batches=%u rx_frames=%u tx_batches=%u tx_frames=%u\n",
		b43_defer_splice, atomic_read(&wl->defer_splice_batches[0]),
		atomic_read(&wl->defer_splice_frames[0]), atomic_read(&wl->defer_splice_batches[1]),
		atomic_read(&wl->defer_splice_frames[1]));
 {
  static const char * const names[B43_DMA_COST_COUNT] = {
   "header", "alloc", "map", "replace", "unmap", "handoff", "clock"
  };
  struct b43_dma_cost *cost = &dev->stall_diag.dma_cost;
  unsigned int i;
  n += sysfs_emit_at(buf, n,
   "dma_cost_schema=1 enabled=%u divisor=%u limit=%u rx_calls=%u sampled=%u\n",
   b43_dma_cost_enabled, B43_DMA_COST_DIVISOR, B43_DMA_COST_LIMIT,
   cost->rx_calls, cost->sampled);
  for (i = 0; i < B43_DMA_COST_COUNT; i++) {
   struct b43_dma_cost_span *p = &cost->span[i];
   n += sysfs_emit_at(buf, n,
    "dma_cost_%s calls=%u ns=%llu max_ns=%u\n",
    names[i], p->calls, (unsigned long long)p->ns, p->max_ns);
  }
 }
 {
  struct b43_dmaring *ring = dev->dma.rx_ring;
  struct page_pool *pool = ring ? ring->rx_pool : NULL;
  struct b43_rx_pool_diag *p = &dev->stall_diag.rx_pool;
  n += sysfs_emit_at(buf, n,
   "rx_pool_schema=1 requested=%u active=%u order=%u page_bytes=%u buffer_bytes=%u headroom=%u max_pages=%u hold=%u release=%u allocated=%u prepared=%u received=%u alloc_fail=%u build_fail=%u bad_addr=%u cap_fail=%u ring_released=%u\n",
   b43_rx_pool, !!pool, pool ? ring->rx_pool_order : 0,
   pool ? ring->rx_pool_bytes : 0, ring ? ring->rx_buffersize : 0,
   NET_SKB_PAD, B43_RX_POOL_MAX_PAGES,
   pool ? READ_ONCE(pool->pages_state_hold_cnt) : 0,
   pool ? (u32)atomic_read(&pool->pages_state_release_cnt) : 0,
   p->allocated, p->prepared, p->received, p->alloc_fail,
   p->build_fail, p->bad_addr, p->cap_fail, p->released);
 }
	n += sysfs_emit_at(buf, n, "rx_direction_schema=1 requested_bidir=%u active_bidir=%u\n",
		b43_rx_bidir, dev->dma.rx_ring &&
		dev->dma.rx_ring->rx_dma_dir == DMA_BIDIRECTIONAL);
	mutex_unlock(&wl->mutex);
	return n;
}
static DEVICE_ATTR_RO(stall_defer);

int b43_stall_diag_register(struct b43_wldev *dev)
{
	unsigned int i;
	int err;

	if (!b43_stall_diag_enabled || dev->phy.type != B43_PHYTYPE_AC)
		return 0;
	for (i = 0; i < B43_SD_COUNT; i++)
		atomic_set(&dev->stall_diag.count[i], 0);
	for (i = 0; i < 16; i++)
		atomic_set(&dev->stall_diag.ba_win[i], 0);
	err = device_create_file(dev->dev->dev, &dev_attr_stall_diag);
	if (err)
		return err;
	err = device_create_file(dev->dev->dev, &dev_attr_stall_irqcfg);
	if (err) {
		device_remove_file(dev->dev->dev, &dev_attr_stall_diag);
		return err;
	}
	err = device_create_file(dev->dev->dev, &dev_attr_stall_rxboundary);
	if (err) {
		device_remove_file(dev->dev->dev, &dev_attr_stall_irqcfg);
		device_remove_file(dev->dev->dev, &dev_attr_stall_diag);
		return err;
	}
	err = device_create_file(dev->dev->dev, &dev_attr_stall_defer);
	if (err) {
		device_remove_file(dev->dev->dev, &dev_attr_stall_rxboundary);
		device_remove_file(dev->dev->dev, &dev_attr_stall_irqcfg);
		device_remove_file(dev->dev->dev, &dev_attr_stall_diag);
		return err;
	}
	dev->stall_diag.registered = true;
	return 0;
}

void b43_stall_diag_unregister(struct b43_wldev *dev)
{
	if (dev->stall_diag.registered) {
		device_remove_file(dev->dev->dev, &dev_attr_stall_defer);
		device_remove_file(dev->dev->dev, &dev_attr_stall_rxboundary);
		device_remove_file(dev->dev->dev, &dev_attr_stall_irqcfg);
		device_remove_file(dev->dev->dev, &dev_attr_stall_diag);
		dev->stall_diag.registered = false;
	}
}
