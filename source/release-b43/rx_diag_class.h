/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Header-only diagnostic classification. No policy or payload access. */
#ifndef B43_RX_DIAG_CLASS_H
#define B43_RX_DIAG_CLASS_H
#ifdef B43_RX_DIAG_NATIVE
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
#else
#include <linux/types.h>
#endif
#define B43_RXD_BEACON 1U
#define B43_RXD_OWN_DATA 2U
#define B43_RXD_GROUP_DATA 4U
#define B43_RXD_OTHER_DATA 8U
#define B43_RXD_PROTECTED 16U
static inline unsigned int b43_rx_diag_classify(const u8 *frame,
                                               unsigned int length,
                                               const u8 *own)
{
 unsigned int i, own_match = 1, result = 0;
 u16 fc;
 if (length < 10)
  return 0;
 fc = (u16)frame[0] | ((u16)frame[1] << 8);
 if ((fc & 0x00fc) == 0x0080)
  result |= B43_RXD_BEACON;
 if ((fc & 0x000c) != 0x0008)
  return result;
 if (fc & 0x4000)
  result |= B43_RXD_PROTECTED;
 if (frame[4] & 1)
  return result | B43_RXD_GROUP_DATA;
 for (i = 0; i < 6; i++)
  own_match &= frame[4+i] == own[i];
 return result | (own_match ? B43_RXD_OWN_DATA : B43_RXD_OTHER_DATA);
}
#endif
