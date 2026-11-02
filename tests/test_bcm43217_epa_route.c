// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t u32;
#define CPTCFG_B43_BCMA 1
#define B43_BUS_BCMA 1
#define BCMA_CHIP_ID_BCM43217 43217
#define NL80211_BAND_2GHZ 0
#define BCMA_CC_PMU_CHIPCTL_ADDR 0x650
#define BCMA_CC_PMU_CHIPCTL_DATA 0x654
struct bcma_drv_cc {u32 data;unsigned addr,changes;};
struct bcma_bus {struct bcma_drv_cc drv_cc;};
struct bcma_device {struct bcma_bus *bus;};
struct bus {unsigned bus_type,chip_id;struct bcma_device*bdev;};
struct phy {unsigned rev,radio_ver,radio_rev,radio_version;};
struct ssb_sprom {unsigned boardflags2_hi;struct {struct {unsigned extpa_gain;}ghz2;}fem;};
struct b43_wldev {struct phy phy;struct bus*dev;void*wl;};
static unsigned ipa,band;
static unsigned b43_nphy_ipa(struct b43_wldev*d){(void)d;return ipa;}
static unsigned b43_current_band(void*w){(void)w;return band;}
#define b43info(w,f,...) do {(void)(w);(void)before;(void)after;} while(0)
static void bcma_pmu_write32(struct bcma_drv_cc*c,unsigned r,unsigned v){assert(r==0x650&&v==1);c->addr=v;}
static unsigned bcma_pmu_read32(struct bcma_drv_cc*c,unsigned r){assert(r==0x650||r==0x654);return r==0x650?c->addr:c->data;}
static void bcma_chipco_chipctl_maskset(struct bcma_drv_cc*c,unsigned a,unsigned m,unsigned v){assert(a==1&&m==~0x2100U&&v==0x2100);bcma_pmu_write32(c,0x650,a);bcma_pmu_read32(c,0x650);c->data=(c->data&m)|v;c->changes++;}
static void actual(struct b43_wldev*dev,struct ssb_sprom*sprom){struct phy*phy=&dev->phy;
	/* EPA17_PIN_ROUTE_BEGIN
	 * BCM43217's two external-PA control outputs require the dedicated
	 * PMU ChipControl1 mux bits, not the old package9/10 ChipCommon mux.
	 * Vendor907.111 wlc_phy_init_nphy 0x11c5a0..0x11c5d0 enables
	 * CCTRL43217_EXTPA_C0/C1 unless boardflags2 bit20 excludes them.
	 * sbchipc.h identifies core0 bit13 and core1 bit8. Use the factory
	 * EPA board selector and preserve every unrelated register bit.
	 * This routes normal PHY PA controls; no forced TX enable, gain,
	 * calibration, transmit target or regulatory power-limit change.
	 */
	if (dev->dev->bus_type == B43_BUS_BCMA &&
	    dev->dev->chip_id == BCMA_CHIP_ID_BCM43217 &&
	    phy->rev == 17 && phy->radio_ver == 0x2057 &&
	    phy->radio_rev == 14 && phy->radio_version == 1 &&
	    !b43_nphy_ipa(dev) && sprom->fem.ghz2.extpa_gain == 3 &&
	    !(sprom->boardflags2_hi & 0x0010) &&
	    b43_current_band(dev->wl) == NL80211_BAND_2GHZ) {
#ifdef CPTCFG_B43_BCMA
		struct bcma_drv_cc *cc = &dev->dev->bdev->bus->drv_cc;

		bcma_chipco_chipctl_maskset(cc, 1, ~0x00002100U, 0x00002100U);
#endif
	}
	/* EPA17_PIN_ROUTE_END */


}
int main(void){struct bcma_bus bb;struct bcma_device bd={.bus=&bb};struct bus b={.bdev=&bd};struct b43_wldev d={.dev=&b};struct ssb_sprom s;unsigned long cases=0;
for(unsigned mode=0;mode<1024;mode++)for(unsigned seed=0;seed<64;seed++){
b.bus_type=(mode&1)?1:0;b.chip_id=(mode&2)?43217:43227;d.phy.rev=(mode&4)?17:16;
d.phy.radio_ver=(mode&8)?0x2057:0x2056;d.phy.radio_rev=(mode&16)?14:13;d.phy.radio_version=(mode&32)?1:0;
ipa=(mode&64)?0:1;s.fem.ghz2.extpa_gain=(mode&128)?3:2;s.boardflags2_hi=(mode&256)?0:0x10;band=(mode&512)?0:1;
u32 saved=0x9e3779b9U*seed;bb.drv_cc=(struct bcma_drv_cc){.data=saved,.addr=7};actual(&d,&s);
assert(bb.drv_cc.changes==(mode==1023));assert(bb.drv_cc.data==(mode==1023?(saved|0x2100U):saved));
assert(bb.drv_cc.addr==(mode==1023?1U:7U));cases++;}
printf("PASS %lu actual EPA-route selector, bit preservation, no-write fallback cases\n",cases);}
