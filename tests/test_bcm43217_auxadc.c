// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16;
struct ssb_sprom {struct {struct {unsigned extpa_gain,pdet_range;} ghz2;} fem;};
struct phy {unsigned rev,radio_ver,radio_rev,radio_version;};
struct bus {unsigned chip_id;};
struct b43_wldev {struct phy phy;struct bus*dev;void*wl;};
#define BCMA_CHIP_ID_BCM43217 43217
#define NL80211_BAND_2GHZ 0
#define B43_NTAB16(t,a) (((t)<<16)|(a))
static unsigned band,ipa,writes;static u16 memory[1024],saved[1024];
static unsigned b43_current_band(void*w){(void)w;return band;}
static unsigned b43_nphy_ipa(struct b43_wldev*d){(void)d;return ipa;}
#define b43info(w,fmt,...) do {(void)(w);}while(0)
static void b43_ntab_write_bulk(struct b43_wldev*d,unsigned tag,unsigned n,const u16*a){
 (void)d;assert((tag>>16)==8 &&n==4);unsigned k=tag&65535;assert(k+n<=1024);
 for(unsigned i=0;i<n;i++){memory[k+i]=a[i];writes++;}
}
static void actual(struct b43_wldev*dev,struct ssb_sprom*sprom){struct phy *phy=&dev->phy;
	/* AUXADC17_BEGIN */
	if (dev->dev->chip_id == BCMA_CHIP_ID_BCM43217 &&
	    phy->rev == 17 && phy->radio_ver == 0x2057 &&
	    phy->radio_rev == 14 && phy->radio_version == 1 &&
	    !b43_nphy_ipa(dev) && sprom->fem.ghz2.extpa_gain == 3 &&
	    sprom->fem.ghz2.pdet_range == 3 &&
	    b43_current_band(dev->wl) == NL80211_BAND_2GHZ) {
		/* Vendor907.111 wlc_phy_workarounds_nphy:
		 * default ADC arrays .rodata+0x3baa0/3baa8/3bac0/3bac8;
		 * detector range3, 2GHz (0x11831c..0x11834c) uses
		 * VMID0/1[3]=0x89 and gain0/1[3]=0.
		 * Detector setup only: preserve factory power coefficients,
		 * hardware power control, target limits and all other boards.
		 */
		static const u16 vmid0[] = { 0x8e, 0x96, 0x96, 0x89 };
		static const u16 vmid1[] = { 0x8f, 0x9f, 0x9f, 0x89 };
		static const u16 gain[] = { 2, 2, 2, 0 };

		b43_ntab_write_bulk(dev, B43_NTAB16(8, 0x08), 4, vmid0);
		b43_ntab_write_bulk(dev, B43_NTAB16(8, 0x18), 4, vmid1);
		b43_ntab_write_bulk(dev, B43_NTAB16(8, 0x0c), 4, gain);
		b43_ntab_write_bulk(dev, B43_NTAB16(8, 0x1c), 4, gain);
	}
	/* AUXADC17_END */


}
int main(void){
 struct bus b;struct b43_wldev d={.dev=&b};struct ssb_sprom s;unsigned long cases=0;
 for(unsigned mode=0;mode<512;mode++)for(unsigned seed=0;seed<32;seed++){
 b.chip_id=(mode&1)?43217:43227;d.phy.rev=(mode&2)?17:16;d.phy.radio_ver=(mode&4)?0x2057:0x2056;
 d.phy.radio_rev=(mode&8)?14:13;d.phy.radio_version=(mode&16)?1:0;ipa=(mode&32)?0:1;
 s.fem.ghz2.extpa_gain=(mode&64)?3:2;s.fem.ghz2.pdet_range=(mode&128)?3:2;band=(mode&256)?0:1;
 for(unsigned i=0;i<1024;i++)memory[i]=(seed*1049+i*11)&65535;
 memcpy(saved,memory,sizeof(memory));writes=0;actual(&d,&s);assert(writes==(mode==511?16:0));
 static const u16 vm0[]={142,150,150,137},vm1[]={143,159,159,137},g[]={2,2,2,0};
 for(unsigned i=0;i<1024;i++){
 u16 want=saved[i];
 if(mode==511){
 if(i>=8&&i<12)want=vm0[i-8];if(i>=24&&i<28)want=vm1[i-24];
 if(i>=12&&i<16)want=g[i-12];if(i>=28&&i<32)want=g[i-28];
 }assert(memory[i]==want);}cases++;
 }
 printf("PASS %lu actual ADC block selector, constants, write-bounds and untouched-memory cases\n",cases);
}
