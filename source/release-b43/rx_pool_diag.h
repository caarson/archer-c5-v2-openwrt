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
