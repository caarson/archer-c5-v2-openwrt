# Archer C5 v2 b797 — driver source prerelease

Community OpenWrt25.12.6 / Linux6.12.112 integration for the Archer C5 v2 BCM4352 5GHz radio, based on Alessio Ferri’s b43-ac-wip. This source prerelease accompanies a successfully installed private lab firmware image. It is not an official OpenWrt release or a generic flashable download.

## Included

- Exact qualified b797 b43 and CDC mac80211 effective source snapshots, support headers, reviewed deltas, configurations, offline fixtures and provenance.
- Guarded deferred RX/IP batching, cache synchronization correction, bidirectional RX DMA/page pooling, incomplete hardware A-MSDU record guard and separate CCMP-IV guard. Essential diagnostics remain enabled; optional DMA timing is off.
- Sanitized qualification and permanent-installation results.

## Validation

A840.39second mixed-workload RAM qualification passed, including10second receive bursts through200Mbps, upload bursts through50Mbps,11/11 application SHA checks and ongoing SSH/DNS/LuCI/connectivity probes. Peak sampled/gate CPU86.34%; minimum available RAM63.09MiB. No critical ownership/DMA error or persistent stall was observed. Separate earlier exposures completed200Mbps for50seconds and225Mbps for35seconds.

The same driver pair was then integrated into the permanent image, both OS/root writes independently verified, and a cold-power return confirmed. Network configuration and14 checked MTD hashes remained unchanged; SSH, LuCI login, Gigabit Ethernet, authorized Wi-Fi association, DNS and Internet checks passed.

## Limits

**Stable250Mbps has not been qualified.** The historical persistent receive-load stall is not proven fixed. Short functional tests do not establish long-term reliability, ordinary reconnection behavior or production-hosting readiness. Only one usable CPU was observed.

The private provisioned image, passwords/keys, calibration/flash dumps, raw household captures and proprietary Broadcom microcode are not release assets. A generic public firmware build/installer still needs clean provisioning, reproducible full-image integration and device-specific recovery validation. Consult docs/BUILD.md and docs/RECOVERY.md before attempting integration.

Credit and per-file licenses are preserved; imported AC-PHY support is not represented as newly authored from scratch.
