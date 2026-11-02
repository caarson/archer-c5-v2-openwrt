
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
typedef uint64_t u64;typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#define __force
#define PHASE3_RX_REASON_MAX 64U
#define ETH_ALEN 6
#define NL80211_IFTYPE_STATION 2
#define PAGE_SIZE 4096
#define READ_ONCE(x) (x)
#define module_param_named(...)
#define MODULE_PARM_DESC(...)
#define module_param_cb(...)
#define DEFINE_SPINLOCK(n) int n
static bool snapshot_locked,agg_locked;
#define spin_lock_irqsave(lock,flags) do{(void)(lock);flags=0;assert(!snapshot_locked);snapshot_locked=true;}while(0)
#define spin_unlock_irqrestore(lock,flags) do{(void)(lock);(void)(flags);assert(snapshot_locked);snapshot_locked=false;}while(0)
typedef struct{int v;}atomic_t;
#define atomic_add(n,p) ((p)->v+=(n))
#define atomic_inc(p) ((p)->v++)
#define atomic_read(p) ((p)->v)
typedef u32 ieee80211_rx_result;
#define RX_CONTINUE 1
#define RX_QUEUED 2
#define RX_DROP 100
struct kernel_param{};
struct kernel_param_ops{int(*get)(char*,const struct kernel_param*);};
struct timer_list{bool pending;unsigned long expires;};
static int timer_pending(struct timer_list*t){assert(agg_locked);return t->pending;}
struct tid_ampdu_rx{u16 head_seq_num,stored_mpdu_num,buf_size;u8 tid;struct timer_list reorder_timer;};
struct ieee80211_sub_if_data{struct{int type;u8 addr[6];}vif;};
struct ieee80211_hdr {u16 frame_control,duration;u8 addr1[6],addr2[6],addr3[6];u16 seq_ctrl;};
struct ieee80211_rx_status {u64 flag;};
struct sk_buff {unsigned int len,capacity;u8 *data;struct ieee80211_rx_status status;bool copy_fail;};
struct key_fixture {u32 color;struct {u32 flags,cipher;u8 keyidx;}conf;};
struct ieee80211_rx_data{void*sta;struct ieee80211_sub_if_data*sdata;struct sk_buff *skb;struct key_fixture *key;int security_idx;};
static unsigned long jiffies;
static bool ether_addr_equal(const u8*a,const u8*b){return !memcmp(a,b,6);}
static int scnprintf(char*b,size_t n,const char*fmt,...){assert(n>0&&n<=PAGE_SIZE);va_list a;va_start(a,fmt);int r=vsnprintf(b,n,fmt,a);va_end(a);assert(r>=0&&(size_t)r<n);return r;}
#define HZ 100
#define IEEE80211_SKB_RXCB(skb) (&(skb)->status)
#define RX_FLAG_IV_STRIPPED (1ULL<<4)
#define IEEE80211_KEY_FLAG_PAIRWISE 1
#define IEEE80211_NUM_TIDS 16
#define le16_to_cpu(x) (x)
static unsigned int fixture_hdrlen=26;
static int ieee80211_hdrlen(u16 fc){(void)fc;return fixture_hdrlen;}
static bool ieee80211_has_protected(u16 fc){return fc&0x4000;}
static void ether_addr_copy(u8*a,const u8*b){memcpy(a,b,6);}
static int skb_copy_bits(struct sk_buff*s,int off,void*out,int len){if(s->copy_fail||off<0||len<0||(unsigned)off+len>s->capacity)return -1;memcpy(out,s->data+off,len);return 0;}
#include "../source/release-mac80211/phase3_reorder_diag.h"

#include "../source/release-mac80211/phase3_reorder_trigger.h"
struct phase3_pn_record {
	u64 flags;
	u32 observed_jiffies, key_color, cipher, length;
	u16 sequence, frame_control, header_length;
	u8 queue, key_index, kind;
	u8 pn[6], previous[6], iv[8];
};
static bool phase3_pn_diag;
#include "../source/release-mac80211/phase3_trigger_pn.h"

int main(void){
 unsigned cases=0;
 struct ieee80211_sub_if_data s={.vif={.type=2,.addr={0x62,0xe3,0x27,0x3c,0x51,0xa8}}};
 struct tid_ampdu_rx a={.tid=0,.buf_size=64};
 u8 buffer[64]={0}, iv[8]={0x11,0x22,0,0x20,0x33,0x44,0x55,0x66};
 struct ieee80211_hdr *hdr=(void*)buffer;hdr->frame_control=0x4288;hdr->seq_ctrl=298<<4;
 memset(hdr->addr1,0x62,6);memset(hdr->addr2,0xdc,6);memcpy(buffer+26,iv,8);
 struct sk_buff skb={.len=1550,.capacity=64,.data=buffer,.status={.flag=2}};
 for(int enable=0;enable<2;enable++)for(int type=0;type<3;type++)for(int tid=0;tid<16;tid++)for(unsigned len=0;len<70;len++)for(int strip=0;strip<2;strip++){
  memset(&phase3_reorder_trigger,0,sizeof(phase3_reorder_trigger));phase3_reorder_diag=enable;s.vif.type=type;a.tid=tid;skb.len=len;skb.status.flag=2|(strip?RX_FLAG_IV_STRIPPED:0);jiffies=0xfffffff0;
  phase3_reorder_trigger_capture(&s,&a,&skb,298,2804,235);
  bool valid=enable&&type==2&&tid==0&&len>=24;
  assert(phase3_reorder_trigger.valid==valid);
  if(valid){assert(phase3_reorder_trigger.incoming==298&&phase3_reorder_trigger.before==2804&&phase3_reorder_trigger.predicted==235);assert(phase3_reorder_trigger.iv_present==(!strip&&len>=34));}
  cases++;
 }
 phase3_reorder_diag=true;s.vif.type=2;a.tid=0;skb.len=1550;skb.status.flag=2;memset(&phase3_reorder_trigger,0,sizeof(phase3_reorder_trigger));jiffies=0xfffffff0;
 phase3_reorder_trigger_capture(&s,&a,&skb,298,2804,235);assert(phase3_reorder_trigger.iv_present);cases++;
 struct phase3_reorder_trigger saved=phase3_reorder_trigger;
 jiffies=0x20;assert(phase3_reorder_trigger_matches(&s,0,hdr->seq_ctrl,iv));cases++;
 assert(!phase3_reorder_trigger_matches(&s,1,hdr->seq_ctrl,iv));cases++;
 assert(!phase3_reorder_trigger_matches(&s,0,hdr->seq_ctrl+16,iv));cases++;
 u8 bad[8];memcpy(bad,iv,8);bad[7]^=1;assert(!phase3_reorder_trigger_matches(&s,0,hdr->seq_ctrl,bad));cases++;
 jiffies=0x55;assert(!phase3_reorder_trigger_matches(&s,0,hdr->seq_ctrl,iv));cases++;
 phase3_reorder_trigger_capture(&s,&a,&skb,999,1,2);assert(!memcmp(&saved,&phase3_reorder_trigger,sizeof(saved)));cases++;
 char output[PAGE_SIZE+16];memset(output,0xcc,sizeof(output));int n=phase3_reorder_trigger_get(output,0);assert(n>0&&n<600&&strstr(output,"incoming=298 before=2804 predicted=235")&&strstr(output,"iv=1122002033445566"));for(unsigned i=PAGE_SIZE;i<sizeof(output);i++)assert((u8)output[i]==0xcc);cases++;
 struct key_fixture key={.color=1,.conf={.flags=1,.cipher=0xfac04,.keyidx=0}};struct ieee80211_rx_data rx={.sta=&s,.sdata=&s,.skb=&skb,.key=&key,.security_idx=0};u8 pn[6]={0,0,0,3,1,2},previous[6]={0,0,0,3,9,8};
 jiffies=0x20;phase3_pn_diag=true;
 for(int reason=0;reason<3;reason++){
  phase3_trigger_pn_valid=false;memset(&phase3_trigger_pn,0,sizeof(phase3_trigger_pn));
  phase3_reorder_pn_observe(&rx,reason==2?NULL:pn,reason==2?NULL:previous,iv,26,reason);
  assert(phase3_trigger_pn_valid&&phase3_trigger_pn.kind==reason&&phase3_trigger_pn_present==(reason!=2)&&phase3_trigger_pn.key_color==1);
  if(reason!=2)assert(!memcmp(phase3_trigger_pn.pn,pn,6)&&!memcmp(phase3_trigger_pn.previous,previous,6));
  struct phase3_pn_record first=phase3_trigger_pn;phase3_reorder_pn_observe(&rx,pn,pn,iv,26,0);assert(!memcmp(&first,&phase3_trigger_pn,sizeof(first)));cases++;
 }
 for(int invalid=0;invalid<8;invalid++){
  phase3_trigger_pn_valid=false;phase3_pn_diag=true;rx.key=&key;key.conf.flags=1;rx.security_idx=0;skb.len=1550;int hdrlen=26;int reason=0;
  if(invalid==0)phase3_pn_diag=false;if(invalid==1)rx.key=NULL;if(invalid==2)key.conf.flags=0;if(invalid==3)rx.security_idx=-1;if(invalid==4)rx.security_idx=17;if(invalid==5)hdrlen=23;if(invalid==6)reason=3;if(invalid==7)skb.len=33;
  phase3_reorder_pn_observe(&rx,pn,previous,iv,hdrlen,reason);assert(!phase3_trigger_pn_valid);cases++;
 }
 phase3_pn_diag=true;rx.key=&key;key.conf.flags=1;rx.security_idx=0;skb.len=1550;phase3_trigger_pn_valid=false;
 phase3_reorder_pn_observe(&rx,pn,previous,iv,26,1);memset(output,0xcc,sizeof(output));int m=phase3_trigger_pn_get(output,0);assert(m>0&&m<600&&strstr(output,"valid=1 reason=1")&&strstr(output,"pn=000000030102 previous=000000030908"));for(unsigned i=PAGE_SIZE;i<sizeof(output);i++)assert((u8)output[i]==0xcc);cases++;
 assert(!snapshot_locked);
 printf("{\"PASS\":true,\"cases\":%u,\"reorder_trigger_page_bytes\":%d,\"PN_trigger_page_bytes\":%d,\"new_state_bytes\":%zu,\"actual_source_helpers\":true,\"API_standins\":true,\"not_hardware_or_kernel_concurrency_proof\":true}\n",cases,n,m,sizeof(phase3_reorder_trigger)+sizeof(phase3_trigger_pn)+sizeof(phase3_trigger_pn_valid)+sizeof(phase3_trigger_pn_present));
}
