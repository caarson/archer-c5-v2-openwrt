
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
typedef uint32_t u32;typedef uint64_t u64;typedef uint16_t u16;
typedef uint64_t dma_addr_t;typedef int gfp_t;
#define PAGE_SIZE 4096U
#define NET_SKB_PAD 64U
#define SKB_DATA_ALIGN(n) (((n)+63U)&~63U)
#define NUMA_NO_NODE -1
#define PP_FLAG_DMA_MAP 1U
#define GFP_KERNEL 0
#define GFP_ATOMIC 1
#define DMA_BIDIRECTIONAL 0
#define DMA_TO_DEVICE 1
#define DMA_FROM_DEVICE 2
#define B43_PHYTYPE_AC 11
#define B43_DMA_64BIT 64
#define DMA_ATTR_SKIP_CPU_SYNC 1
#define B43_WARN_ON(x) assert(!(x))
#define unlikely(x) (x)
#define IS_ERR(x) ((uintptr_t)(x)>UINTPTR_MAX-4095)
#define PTR_ERR(x) ((int)(intptr_t)(x))
#define WRITE_ONCE(x,y) ((x)=(y))
#define atomic_read(x) (*(x))
#define atomic_inc(x) (++*(x))
#define mb() ((void)0)
#define b43err(...) ((void)0)
struct skb_shared_info {char opaque[248];};struct b43_plcp_hdr6 {char opaque[6];};
struct page_pool;struct page {unsigned char *cpu,*dev;dma_addr_t dma;unsigned refs;bool cpu_owned,posted;struct page_pool*pool;};
struct sk_buff {struct page*page;unsigned char*data;bool recycle;unsigned len;};
struct page_pool_params {unsigned order,pool_size;int nid;void*dev;int dma_dir;unsigned offset,max_len,flags;};
struct page_pool {struct page_pool_params p;u32 pages_state_hold_cnt;int pages_state_release_cnt;struct page*pages[1200];unsigned count;bool destroying;};
/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_RX_POOL_DIAG_H
#define B43_RX_POOL_DIAG_H
#define B43_RX_POOL_MAX_PAGES 1024U
struct b43_rx_pool_diag {
 u32 allocated, prepared, received, alloc_fail, build_fail, bad_addr;
 u32 cap_fail, released;
};
extern bool b43_rx_pool;
#endif
/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_DMA_COST_H_
#define B43_DMA_COST_H_

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
static struct page*currently_building;

bool b43_rx_pool=true,b43_rx_sync_len=true;static unsigned clock_ns;
static unsigned livepools,livepages,liveskbs;static unsigned request_alloc,request_build;
static unsigned fail_alloc_at,fail_build_at;static bool fail_create,bad_address;
static unsigned calls_cpu,calls_device,post_count,unmaps;
struct wl {int rxsync_headers,rxsync_payloads,rxsync_skips;unsigned rxsync_lastlen;void*hw;};
struct wldev {struct {int type;}phy;struct {struct b43_rx_pool_diag rx_pool;struct b43_dma_cost dma_cost;}stall_diag;struct {void*dma_dev;} bus;struct {void*dma_dev;} *dev;struct wl*wl;};
struct b43_dmaring;struct b43_dmadesc_generic {int slot;};
struct b43_dmadesc_meta {struct sk_buff*skb;dma_addr_t dmaaddr;};
struct dma_ops {void(*fill_descriptor)(struct b43_dmaring*,struct b43_dmadesc_generic*,dma_addr_t,u16,int,int,int);struct b43_dmadesc_generic*(*idx2desc)(struct b43_dmaring*,int,struct b43_dmadesc_meta**);};
struct b43_dmaring {struct wldev*dev;bool tx;int type,index,nr_slots,used_slots,current_slot;unsigned frameoffset,rx_buffersize;int rx_dma_dir;struct page_pool*rx_pool;unsigned rx_pool_order,rx_pool_bytes;struct dma_ops*ops;struct b43_dmadesc_meta meta[192];struct b43_dmadesc_generic desc[192];};
static unsigned get_order(unsigned n){unsigned k=0;while((PAGE_SIZE<<k)<n)k++;return k;}
static u64 ktime_get_mono_fast_ns(void){return clock_ns+=100;}
static struct page_pool*page_pool_create(struct page_pool_params*p){
 if(fail_create)return(void*)(intptr_t)-ENOMEM;
 assert(p->order==1&&p->pool_size==192&&p->dma_dir==DMA_BIDIRECTIONAL&&p->flags==PP_FLAG_DMA_MAP&&p->offset==NET_SKB_PAD&&p->max_len==3943);
 struct page_pool*q=calloc(1,sizeof(*q));assert(q);q->p=*p;livepools++;return q;}
static void final_pool(struct page_pool*p){
 for(unsigned i=0;i<p->count;i++){struct page*q=p->pages[i];assert(!q->refs);free(q->cpu);free(q->dev);free(q);livepages--;p->pages_state_release_cnt++;}
 assert((u32)p->pages_state_release_cnt==p->pages_state_hold_cnt);free(p);livepools--;}
static struct page*page_pool_alloc_pages(struct page_pool*p,gfp_t gfp){
 (void)gfp;request_alloc++;if(fail_alloc_at==request_alloc)return NULL;assert(!p->destroying);
 for(unsigned i=0;i<p->count;i++)if(!p->pages[i]->refs){struct page*q=p->pages[i];q->refs=1;q->posted=false;currently_building=q;return q;}
 assert(p->count<1200);struct page*q=calloc(1,sizeof(*q));assert(q);q->cpu=malloc(8192);q->dev=malloc(8192);assert(q->cpu&&q->dev);memset(q->cpu,0x61,8192);memset(q->dev,0x13,8192);q->dma=bad_address?UINT64_MAX-31:(p->count+1U)*65536ULL;q->pool=p;q->refs=1;p->pages[p->count++]=q;p->pages_state_hold_cnt++;livepages++;currently_building=q;return q;}
static void page_pool_put_full_page(struct page_pool*p,struct page*q,bool direct){assert(!direct&&q->pool==p&&q->refs);if(!--q->refs&&p->destroying){bool any=false;for(unsigned i=0;i<p->count;i++)any|=p->pages[i]->refs!=0;if(!any)final_pool(p);}}
static void page_pool_destroy(struct page_pool*p){assert(!p->destroying);p->destroying=true;for(unsigned i=0;i<p->count;i++)if(p->pages[i]->refs)return;final_pool(p);}
static void*page_address(struct page*p){return p->cpu;}
static dma_addr_t page_pool_get_dma_addr(struct page*p){return p->dma;}

static struct sk_buff*build_skb(void*data,unsigned bytes){
 request_build++;assert(bytes==8192);if(fail_build_at==request_build)return NULL;
 struct page_pool*p=currently_building->pool;struct page*q=NULL;for(unsigned i=0;i<p->count;i++)if(p->pages[i]->cpu==data)q=p->pages[i];assert(q&&q->refs==1);
 struct sk_buff*s=calloc(1,sizeof(*s));assert(s);s->page=q;s->data=data;liveskbs++;return s;}
static void skb_mark_for_recycle(struct sk_buff*s){s->recycle=true;}
static void skb_reserve(struct sk_buff*s,unsigned n){assert(n==NET_SKB_PAD);s->data+=n;}
static void dev_kfree_skb_any(struct sk_buff*s){assert(s->recycle);struct page*q=s->page;free(s);liveskbs--;page_pool_put_full_page(q->pool,q,false);}
#define dev_kfree_skb(s) dev_kfree_skb_any(s)
static struct page*lookup(struct b43_dmaring*r,dma_addr_t addr){struct page_pool*p=r->rx_pool;for(unsigned i=0;i<p->count;i++)if(p->pages[i]->dma+NET_SKB_PAD==addr)return p->pages[i];abort();}
static void sync_descbuffer_for_cpu(struct b43_dmaring*r,dma_addr_t addr,size_t n){assert(r->rx_dma_dir==DMA_BIDIRECTIONAL&&n<=3943);struct page*q=lookup(r,addr);memcpy(q->cpu+NET_SKB_PAD,q->dev+NET_SKB_PAD,n);q->cpu_owned=true;calls_cpu++;}
static void sync_descbuffer_for_device(struct b43_dmaring*r,dma_addr_t addr,size_t n){assert(r->rx_dma_dir==DMA_BIDIRECTIONAL&&n==3943);struct page*q=lookup(r,addr);assert(q->cpu_owned);memcpy(q->dev+NET_SKB_PAD,q->cpu+NET_SKB_PAD,n);q->cpu_owned=false;calls_device++;}
static void b43_poison_rx_buffer(struct b43_dmaring*r,struct sk_buff*s){assert(s->page->cpu_owned);s->data[0]=s->data[1]=0;memset(s->data+r->frameoffset,255,8);}
static void fill(struct b43_dmaring*r,struct b43_dmadesc_generic*d,dma_addr_t addr,u16 n,int a,int b,int c){(void)d;assert(!a&&!b&&!c&&n==3943);struct page*q=lookup(r,addr);assert(!q->cpu_owned&&q->dev[NET_SKB_PAD]==0&&q->dev[NET_SKB_PAD+1]==0);for(unsigned i=0;i<8;i++)assert(q->dev[NET_SKB_PAD+r->frameoffset+i]==255);q->posted=true;post_count++;}
static struct b43_dmadesc_generic*idx(struct b43_dmaring*r,int i,struct b43_dmadesc_meta**m){assert(i>=0&&i<192);*m=&r->meta[i];r->desc[i].slot=i;return &r->desc[i];}
static bool b43_rx_short_sync(struct b43_dmaring*r){return b43_rx_sync_len&&r->dev->phy.type==B43_PHYTYPE_AC;}
static void unmap_descbuffer(struct b43_dmaring*r,dma_addr_t addr,size_t n,int tx){(void)addr;(void)n;(void)tx;assert(!r->rx_pool);unmaps++;}
static void dma_unmap_single_attrs(void*dev,dma_addr_t a,size_t n,int dir,unsigned attrs){(void)dev;(void)a;(void)n;assert(dir==DMA_BIDIRECTIONAL&&attrs==DMA_ATTR_SKIP_CPU_SYNC);unmaps++;}
static void free_descriptor_buffer(struct b43_dmaring*r,struct b43_dmadesc_meta*m){assert(!r->tx&&m->skb);dev_kfree_skb_any(m->skb);m->skb=NULL;}
#define b43_dma_ptr_is_poisoned(p) ((p)==(void*)(intptr_t)-ENOMEM)
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
static void b43_rx_unmap_received(struct b43_dmaring *ring, dma_addr_t addr,
				 unsigned int len)
{
	/* Caller has validated len+frameoffset and allocated the replacement. */
	if (b43_rx_short_sync(ring)) {
		sync_descbuffer_for_cpu(ring, addr, len + ring->frameoffset);
		atomic_inc(&ring->dev->wl->rxsync_payloads);
		if (ring->rx_pool) {
			ring->dev->stall_diag.rx_pool.received++;
		} else {
			dma_unmap_single_attrs(ring->dev->dev->dma_dev, addr,
				       ring->rx_buffersize, ring->rx_dma_dir,
				       DMA_ATTR_SKIP_CPU_SYNC);
			atomic_inc(&ring->dev->wl->rxsync_skips);
		}
		WRITE_ONCE(ring->dev->wl->rxsync_lastlen, len);
	} else {
		unmap_descbuffer(ring, addr, ring->rx_buffersize, 0);
	}
}

static void free_all_descbuffers(struct b43_dmaring *ring)
{
	struct b43_dmadesc_meta *meta;
	int i;

	if (!ring->used_slots)
		return;
	for (i = 0; i < ring->nr_slots; i++) {
		/* get meta - ignore returned value */
		ring->ops->idx2desc(ring, i, &meta);

		if (!meta->skb || b43_dma_ptr_is_poisoned(meta->skb)) {
			B43_WARN_ON(!ring->tx);
			continue;
		}
		if (ring->tx) {
			unmap_descbuffer(ring, meta->dmaaddr,
					 meta->skb->len, 1);
		} else {
			if (ring->rx_pool) {
				sync_descbuffer_for_cpu(ring, meta->dmaaddr, ring->rx_buffersize);
				ring->dev->stall_diag.rx_pool.released++;
			} else {
				unmap_descbuffer(ring, meta->dmaaddr,
					 ring->rx_buffersize, 0);
			}
		}
		free_descriptor_buffer(ring, meta);
	}
}

static int setup_rx_descbuffer(struct b43_dmaring*r,struct b43_dmadesc_generic*d,struct b43_dmadesc_meta*m,gfp_t g,bool sample){return b43_pool_setup(r,d,m,g,sample);}
static int alloc_initial_descbuffers(struct b43_dmaring *ring)
{
	int i, err = -ENOMEM;
	struct b43_dmadesc_generic *desc;
	struct b43_dmadesc_meta *meta;

	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->ops->idx2desc(ring, i, &meta);

		err = setup_rx_descbuffer(ring, desc, meta, GFP_KERNEL, false);
		if (err) {
			b43err(ring->dev->wl,
			       "Failed to allocate initial descbuffers\n");
			goto err_unwind;
		}
	}
	mb();
	ring->used_slots = ring->nr_slots;
	err = 0;
      out:
	return err;

      err_unwind:
	for (i--; i >= 0; i--) {
		desc = ring->ops->idx2desc(ring, i, &meta);

		if (!ring->rx_pool)
			unmap_descbuffer(ring, meta->dmaaddr, ring->rx_buffersize, 0);
		else
			sync_descbuffer_for_cpu(ring, meta->dmaaddr, ring->rx_buffersize);
		dev_kfree_skb(meta->skb);
		meta->skb = NULL;
	}
	goto out;
}

static struct dma_ops ops={.fill_descriptor=fill,.idx2desc=idx};
static void initring(struct b43_dmaring*r,struct wldev*v,struct wl*w){memset(r,0,sizeof(*r));memset(v,0,sizeof(*v));memset(w,0,sizeof(*w));v->phy.type=B43_PHYTYPE_AC;v->dev=(void*)&v->bus;v->wl=w;r->dev=v;r->type=B43_DMA_64BIT;r->nr_slots=192;r->rx_buffersize=3943;r->frameoffset=40;r->rx_dma_dir=DMA_BIDIRECTIONAL;r->ops=&ops;}
static void reset_faults(void){fail_alloc_at=fail_build_at=0;fail_create=bad_address=false;request_alloc=request_build=0;}
static void clean(struct b43_dmaring*r){free_all_descbuffers(r);b43_pool_destroy(r);assert(!livepools&&!livepages&&!liveskbs);}
int main(void){unsigned cases=0;struct b43_dmaring r;struct wldev v;struct wl w;
 for(unsigned mode=0;mode<5;mode++){
  initring(&r,&v,&w);reset_faults();b43_rx_pool=true;b43_rx_sync_len=true;
  if(mode==0)r.type=32;if(mode==1)r.index=1;if(mode==2)r.rx_dma_dir=DMA_FROM_DEVICE;if(mode==3)b43_rx_sync_len=false;if(mode==4)r.rx_buffersize=16000;
  assert(b43_pool_create(&r)<0&&!r.rx_pool);assert(!livepools);cases++;
 }
 b43_rx_sync_len=true;initring(&r,&v,&w);b43_rx_pool=false;assert(!b43_pool_create(&r)&&!r.rx_pool);b43_rx_pool=true;cases++;
 initring(&r,&v,&w);fail_create=true;assert(b43_pool_create(&r)==-ENOMEM&&!r.rx_pool);reset_faults();cases++;
 /* Every partial initial-fill allocation and skb-build failure unwinds exactly
  * the already-published descriptors, then pool destruction frees mappings.
  */
 for(unsigned kind=0;kind<2;kind++)for(unsigned at=1;at<=192;at++){
  initring(&r,&v,&w);reset_faults();assert(!b43_pool_create(&r));if(kind)fail_build_at=at;else fail_alloc_at=at;
  assert(alloc_initial_descbuffers(&r)<0&&r.used_slots==0);for(unsigned i=0;i<at-1;i++)assert(!r.meta[i].skb);b43_pool_destroy(&r);assert(!livepools&&!livepages&&!liveskbs);cases++;
 }
 reset_faults();initring(&r,&v,&w);assert(!b43_pool_create(&r));bad_address=true;struct b43_dmadesc_meta old={.skb=(void*)0x1234,.dmaaddr=0x5678};struct b43_dmadesc_generic d={0};
 assert(b43_pool_setup(&r,&d,&old,0,true)==-EIO&&old.skb==(void*)0x1234&&old.dmaaddr==0x5678&&v.stall_diag.rx_pool.bad_addr==1);b43_pool_destroy(&r);assert(!livepages&&!liveskbs&&!livepools);cases++;
 reset_faults();initring(&r,&v,&w);assert(!b43_pool_create(&r));r.rx_pool->pages_state_hold_cnt=1024;assert(b43_pool_setup(&r,&d,&old,0,false)==-ENOMEM&&v.stall_diag.rx_pool.cap_fail==1);r.rx_pool->pages_state_hold_cnt=0;b43_pool_destroy(&r);cases++;
 /* Model asynchronous downstream/cloned ownership: an skb reference keeps its
  * page out of allocation, including after ring destruction. Actual helper
  * executes the CPU-poison-device sequence and descriptor publication.
  */
 for(unsigned round=0;round<16;round++){
  reset_faults();initring(&r,&v,&w);assert(!b43_pool_create(&r));assert(!alloc_initial_descbuffers(&r));assert(r.used_slots==192&&livepages==192);
  struct sk_buff*held[97]={0};unsigned heldn=0;
  for(unsigned i=0;i<3000;i++){
   unsigned slot=i%192;struct b43_dmadesc_meta*meta=&r.meta[slot];struct sk_buff*skb=meta->skb;struct page*page=skb->page;assert(page->posted&&page->refs==1);
   memset(page->dev+NET_SKB_PAD+48,(int)(i&255),1400);page->dev[NET_SKB_PAD]=0xa0;page->dev[NET_SKB_PAD+1]=5;
   struct b43_dmadesc_meta saved=*meta;assert(!b43_pool_setup(&r,&r.desc[slot],meta,1,(i%128)==0));assert(meta->skb->page!=page);
   b43_rx_unmap_received(&r,saved.dmaaddr,1440);assert(page->cpu[NET_SKB_PAD+48]==(i&255)&&!unmaps);
   if(i%31==0&&heldn<97){held[heldn++]=skb;page->refs++;}else dev_kfree_skb_any(skb);
   /* release both skb ownership and a separately held clone only later */
   if(i%37==0&&heldn){struct sk_buff*s=held[--heldn];struct page*q=s->page;dev_kfree_skb_any(s);assert(q->refs==1);page_pool_put_full_page(q->pool,q,false);}
   cases++;
  }
  assert(v.stall_diag.rx_pool.allocated==3192&&v.stall_diag.rx_pool.received==3000);assert(r.rx_pool->pages_state_hold_cnt<400);
  free_all_descbuffers(&r);b43_pool_destroy(&r);assert(!r.rx_pool);
  while(heldn){struct sk_buff*s=held[--heldn];struct page*q=s->page;struct page_pool*p=q->pool;dev_kfree_skb_any(s);assert(q->refs==1);page_pool_put_full_page(p,q,false);}
  assert(!livepools&&!livepages&&!liveskbs);cases++;
 }
 /* Counter wrap-safe mapped-page budget, and no double legacy unmapping. */
 reset_faults();initring(&r,&v,&w);assert(!b43_pool_create(&r));assert(!alloc_initial_descbuffers(&r));clean(&r);cases++;
 printf("PASS cases=%u actual_helper=1 actual_init_unwind=1 actual_receive_sync=1 actual_ring_release=1 no_legacy_unmap=%u\n",cases,unmaps);return 0;}
