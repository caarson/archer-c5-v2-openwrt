/* SPDX-License-Identifier: GPL-2.0-or-later */
#define NTRACE_NATIVE
#include "../source/release-b43/nphy_trace_parse.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static const u8 peer[6]={0x02,0,0,0,0,1};
static void packet(u8 *p) {
 memset(p,0,512);p[0]=0x88;p[1]=0x41;memcpy(p+10,peer,6);
 p[26+3]=0x20;memcpy(p+34,"\xaa\xaa\x03\0\0\0\x08\0",8);
 p[42]=0x45;p[44]=1;p[45]=0;p[51]=6;p[74]=0x80;
 memset(p+94,0xa5,418); /* first application byte follows TCP options */
}
int main(void) {
 alarm(15);
 u8 p[512];struct b43_ntrace_record r;unsigned int i,n;unsigned int seed=42;
 _Static_assert(sizeof(r)==208,"record ABI");
 packet(p);assert(b43_nt_prepare(&r,p,512,peer,1));assert(r.captured==94);
 for(i=94;i<176;i++)assert(r.header[i]==0);
 assert(b43_nt_prepare(&r,p,512,peer,0));assert(r.captured==34);
 for(n=0;n<512;n++){
  u8 *shortp=malloc(n?n:1);memcpy(shortp,p,n);
  if(b43_nt_prepare(&r,shortp,n,peer,1))assert(r.captured<=n && r.captured<=176);
  free(shortp);
 }
 p[10]^=1;assert(!b43_nt_prepare(&r,p,512,peer,1));
 packet(p);p[0]|=0x40;assert(!b43_nt_prepare(&r,p,512,peer,1));
 packet(p);p[48]=0x20;assert(b43_nt_prepare(&r,p,512,peer,1));assert(r.captured==62);
 for(i=0;i<200000;i++) {
  n=rand_r(&seed)%513;u8 *q=malloc(n?n:1);
  for(unsigned int j=0;j<n;j++)q[j]=rand_r(&seed);
  if(n>=24 && (i&1)){q[0]=0x88;q[1]=rand_r(&seed);memcpy(q+10,peer,6);}
  if(b43_nt_prepare(&r,q,n,peer,i&1))assert(r.captured<=n && r.captured<=176);
  free(q);
 }
 puts("PASS: header-only limits, protected/decrypted distinction, fragments, target, truncations, 200000 malformed inputs");
}
