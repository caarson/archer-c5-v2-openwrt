/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_NPHY_TRACE_PARSE_H
#define B43_NPHY_TRACE_PARSE_H
#ifdef NTRACE_NATIVE
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
#include <linux/types.h>
#include <linux/string.h>
#endif
#define B43_NTRACE_CAPACITY 16384U
#define B43_NTRACE_PREFIX 176U
/* ARM trial is little endian. Layout is checked in native and ARM builds.
 * Header bytes exclude application payload. PN/IV bytes are not keys. */
struct b43_ntrace_record {
 u64 rxflags;
 u32 ordinal, ticks, macstat;
 u16 phy0, phy3, channel, wirelen, captured, disposition;
 u8 header[B43_NTRACE_PREFIX];
};
static inline u16 nt_be16(const u8 *p) { return (u16)p[0] << 8 | p[1]; }
/* No skb mutation, allocation, register access, or policy decisions. The
 * caller supplies only the linear head, after PLCP/padding. */
static inline int b43_nt_prepare(struct b43_ntrace_record *r, const u8 *p,
 unsigned int n, const u8 *target, int decrypted)
{
 unsigned int fc, h = 24, cut, ip, ihl, tcp, thl;
 if (n < 24) return 0;
 fc = (unsigned int)p[0] | (unsigned int)p[1] << 8;
 /* Version 0, data-present frame, transmitter is the selected client. */
 if ((fc & 0x000f) != 0x0008 || (fc & 0x0040) ||
     memcmp(p + 10, target, 6)) return 0;
 if ((fc & 0x0300) == 0x0300) h += 6;
 if (fc & 0x0080) { h += 2; if (fc & 0x8000) h += 4; }
 if (n < h) return 0;
 memset(r, 0, sizeof(*r));
 cut = h;
 if (fc & 0x4000) {
  if (n - cut < 8) goto done;
  cut += 8;
  if (!decrypted) goto done;
 }
 if (n - cut < 8 || memcmp(p + cut, "\xaa\xaa\x03\x00\x00\x00\x08\x00", 8))
  goto done;
 ip = cut + 8;
 if (n - ip < 20 || (p[ip] >> 4) != 4) goto done;
 ihl = (p[ip] & 15) * 4;
 if (ihl < 20 || n - ip < ihl || nt_be16(p + ip + 2) < ihl) goto done;
 cut = ip + ihl;
 /* Noninitial or fragmented datagrams retain only IP metadata. */
 if (p[ip + 9] != 6 || (nt_be16(p + ip + 6) & 0x3fff)) goto done;
 tcp = cut;
 if (n - tcp < 20 || nt_be16(p + ip + 2) < ihl + 20) goto done;
 thl = (p[tcp + 12] >> 4) * 4;
 if (thl < 20 || n - tcp < thl || nt_be16(p + ip + 2) < ihl + thl) goto done;
 cut += thl;
done:
 if (cut > B43_NTRACE_PREFIX) cut = B43_NTRACE_PREFIX;
 r->captured = cut;
 memcpy(r->header, p, cut);
 return 1;
}
#endif
