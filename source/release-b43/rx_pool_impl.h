/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_RX_POOL_IMPL_H
#define B43_RX_POOL_IMPL_H
/* RX allocator is serialized by the existing wl->mutex; initial fill occurs
 * before IRQ registration. No direct recycling: skb frees may run elsewhere.
 * Page-pool owns mappings through last skb reference and deferred destruction.
 * Driver explicitly syncs CPU header before poison, then the FULL advertised
 * RX region for device before posting. Pool flags deliberately omit SYNC_DEV:
 * the driver performs this ownership transition after its poison writes.
 */
static int b43_pool_create(struct b43_dmaring *ring)
{
 struct page_pool_params p = { 0 };
 struct page_pool *pool;
 unsigned int size;
 if (!b43_rx_pool || ring->tx || ring->dev->phy.type != B43_PHYTYPE_AC)
  return 0;
 if (ring->type != B43_DMA_64BIT || ring->index != 0 ||
     ring->rx_dma_dir != DMA_BIDIRECTIONAL || !b43_rx_sync_len)
  return -EINVAL;
 size = SKB_DATA_ALIGN(NET_SKB_PAD + ring->rx_buffersize) +
        SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
 ring->rx_pool_order = get_order(size);
 if (ring->rx_pool_order > 1)
  return -E2BIG;
 ring->rx_pool_bytes = PAGE_SIZE << ring->rx_pool_order;
 p.order = ring->rx_pool_order;
 p.pool_size = ring->nr_slots;
 p.nid = NUMA_NO_NODE;
 p.dev = ring->dev->dev->dma_dev;
 p.dma_dir = DMA_BIDIRECTIONAL;
 p.offset = NET_SKB_PAD;
 p.max_len = ring->rx_buffersize;
 p.flags = PP_FLAG_DMA_MAP;
 pool = page_pool_create(&p);
 if (IS_ERR(pool))
  return PTR_ERR(pool);
 ring->rx_pool = pool;
 return 0;
}
static int b43_pool_setup(struct b43_dmaring *ring,
                          struct b43_dmadesc_generic *desc,
                          struct b43_dmadesc_meta *meta, gfp_t gfp,
                          bool sample)
{
 struct b43_rx_pool_diag *d = &ring->dev->stall_diag.rx_pool;
 struct b43_dma_cost *cost = &ring->dev->stall_diag.dma_cost;
 struct page_pool *pool = ring->rx_pool;
 struct page *page;
 struct sk_buff *skb;
 dma_addr_t base, dmaaddr;
 u32 stamp = 0, mapped;
 /* Order1 slow allocation creates one page at a time. Fail closed at8MiB
  * mapped pool storage; no unbounded fallback allocation or silent mode swap.
  */
 mapped = pool->pages_state_hold_cnt -
          (u32)atomic_read(&pool->pages_state_release_cnt);
 if (mapped >= B43_RX_POOL_MAX_PAGES) {
  d->cap_fail++;
  return -ENOMEM;
 }
 if (sample)
  stamp = (u32)ktime_get_mono_fast_ns();
 page = page_pool_alloc_pages(pool, gfp);
 if (unlikely(!page)) {
  d->alloc_fail++;
  return -ENOMEM;
 }
 skb = build_skb(page_address(page), ring->rx_pool_bytes);
 if (unlikely(!skb)) {
  d->build_fail++;
  page_pool_put_full_page(pool, page, false);
  return -ENOMEM;
 }
 skb_mark_for_recycle(skb);
 skb_reserve(skb, NET_SKB_PAD);
 if (sample)
  b43_dma_cost_note(cost, B43_DMA_COST_ALLOC,
                   stamp, (u32)ktime_get_mono_fast_ns());
 base = page_pool_get_dma_addr(page);
 dmaaddr = base + NET_SKB_PAD;
 /* Do not call the legacy error helper: it would unmap pool-owned pages.
  * Pool already checked DMA mapping errors. Only AC DMA64 is admitted above.
  */
 if (unlikely((u64)dmaaddr < (u64)base ||
              (u64)dmaaddr + ring->rx_buffersize < (u64)dmaaddr)) {
  d->bad_addr++;
  dev_kfree_skb_any(skb);
  return -EIO;
 }
 if (sample)
  stamp = (u32)ktime_get_mono_fast_ns();
 sync_descbuffer_for_cpu(ring, dmaaddr,
                         ring->frameoffset + sizeof(struct b43_plcp_hdr6) + 2);
 b43_poison_rx_buffer(ring, skb);
 sync_descbuffer_for_device(ring, dmaaddr, ring->rx_buffersize);
 if (sample)
  b43_dma_cost_note(cost, B43_DMA_COST_MAP,
                   stamp, (u32)ktime_get_mono_fast_ns());
 /* MAP timing now means pool prepare CPU/device sync, not a new map API.
  * Publish meta only after every fallible operation and ownership sync.
  */
 meta->skb = skb;
 meta->dmaaddr = dmaaddr;
 ring->ops->fill_descriptor(ring, desc, dmaaddr,
                            ring->rx_buffersize, 0, 0, 0);
 d->allocated++;
 d->prepared++;
 return 0;
}
static void b43_pool_destroy(struct b43_dmaring *ring)
{
 if (ring->rx_pool) {
  page_pool_destroy(ring->rx_pool);
  ring->rx_pool = NULL;
 }
}
#endif
