/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef B43_NPHY_TRACE_H
#define B43_NPHY_TRACE_H
#include "nphy_trace_parse.h"
struct b43_ntrace {
 u32 used, overflow;
 u8 target[6];
 struct b43_ntrace_record records[B43_NTRACE_CAPACITY];
};
#endif
