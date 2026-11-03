# v0.1.1-618 — working experimental source release

The owner selected this build after the bounded 2.4 GHz functional campaign passed. It remains a prerelease with known intermittent reliability concerns.

## Exact selection

- OpenWrt 25.12.6, Linux 6.12.112, matching existing kernel ABI.
- b43 SHA256: `618552caa7b515954d357f91273576ae716ff26b7e2fc4ef9fb51a82c0b0c7c0`.
- mac80211 SHA256: `cdcf3316e0220fba0939e7274ae399bfe8c7dbb3deb4ad5c6faac8ee933ba172` (unchanged).
- Parent b43: `4130be2b5f191189d6534c4d77a2b1f637b3bd1ec185f1a593e8e249d8436006`.

The only delta from 4130 is an optional bounded N-PHY header recorder: b43.h, sysfs.c, xmit.c and two new headers. No descriptor ownership, queue behavior, error recovery, rate control, crypto policy, RF tables, calibration, or AC DMA/page-pool behavior changes. The earlier PS-Poll and other unsuccessful candidates are not included.

## What passed

The RAM diagnostic build completed **870 seconds active / 878.24 seconds including cleanup**, within a 15-minute budget. All **23 transfers, 175 MB verified**, and all **44 DNS / 44 HTTPS rounds** passed. No persistent stall, disconnection, kernel fault or safety-gate violation was observed. USB-client traffic was paced at 2 and 4 Mbps; Mac traffic at 2 Mbps. These are not throughput-ceiling measurements.

Peak sampled CPU was 37.42%; minimum available memory was 59,668 KiB. Local USB-to-AP ping median/p95 were 1.95/9.82 ms. All 869 observed sequences had replies, with the final in-flight interval censored. The Mac workload was three uploads, not an exact replay of the earlier four-flow failure. A subsequent separate four-flow alternating Mac exposure also passed and is not added to the campaign duration.

## Permanent-use distinction

The RAM campaign enabled the target recorder. Normal permanent use omits `nphy_trace_peer`, disabling recording and avoiding its 3,407,888-byte allocation. The same exact module is selected; permanent reboot and functional checks are recorded separately in `evidence/working-version-installation.json`. This is not a claim that the recorder-disabled configuration completed the same 15-minute campaign.

The lab update replaces only the matching b43 file in the existing persistent overlay and activates it at a normal reboot. It does not reflash the kernel, bootloader, calibration, NVRAM, or saved network configuration. The preceding 4130 module is retained externally for rollback; the read-only ROM contains the older b797 module, so simply removing the overlay file would not restore 4130.

## Reliability limits

The recorder is an observer, **not a fix** for the historical multi-second transfer interruption. Nonzero radio retries and failed transmission counters remain. Independent RF capture confirmed retries and a small number of frame identities absent from the selected driver trace; the selector and incomplete RF recording prevent a precise root-cause claim. All observed test transfers recovered and verified.

Working means usable in the lab with monitoring. It does not establish long-term availability, ordinary-disturbance recovery, stable 250 Mbps forwarding, 802.11n on 2.4 GHz, or PhotonSharp public-production readiness.

## Published scope

Matching effective source, optional-observer delta, header-parser fixture, manifests and curated evidence are published. The historical aggregate patch reconstructs 4130 from the original baseline; the observer delta then reconstructs 618. The current source snapshot already includes both. No private provisioned image, credentials, packet captures, firmware microcode, factory data or generic public flashable firmware is supplied.
