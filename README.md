# Archer C5 v2: OpenWrt and BCM4352 AC-PHY

Community engineering source release for **TP-Link Archer C5 v2**, OpenWrt **25.12.6**, Linux **6.12.112**, and a modified **b43** host driver based on [Alessio Ferri’s b43-ac-wip](https://github.com/aleferri/b43-ac-wip).

**Prerelease / device-specific research.** The qualified b797 configuration has been installed permanently on the development router. Installation and cold-boot checks passed; see `evidence/permanent-installation.json`. This repository provides matching driver source, review diffs, configurations, tests and measured results. It does not supply a generic flashable image or a stock-firmware web-upgrade path. Private provisioned images and proprietary radio microcode are excluded.

## What works in the lab

- BCM4352 5 GHz station uplink, with Internet forwarded to Gigabit Ethernet LAN clients.
- Existing Archer RF front-end adaptation, guarded deferred receive processing, RX/IP batching, RX synchronization correction and page-pool/bidirectional DMA support.
- Incomplete hardware A-MSDU record rejection and a separate mac80211 CCMP-IV guard. These are correctness measures; the historical persistent receive-load stall is **not proven fixed**.
- Essential fault monitoring retained; optional DMA timing disabled in the qualified configuration.

## Evidence, with limits

| Exposure | Result |
|---|---|
| Latest bounded mixed-workload RAM qualification | PASS, 840.39 seconds (14 minutes) |
| Receive forwarding bursts | 10–200 Mbps, 10 seconds each; highest delivered 199.945 Mbps |
| Upload forwarding bursts | 10–50 Mbps; highest delivered 48.856 Mbps |
| Peak sampled / safety-gate CPU | 86.34% |
| Minimum available RAM | 63.09 MiB |
| Application data integrity | 11/11 SHA checks passed; one during a 150 Mbps burst |
| Sampled connectivity / DNS / SSH / LuCI | Passed; no observed disconnect or critical DMA/ownership error |
| Earlier b797 performance trials | 200 Mbps for 50 s; 225 Mbps for 35 s |
| 250 Mbps qualification | **Not achieved**; earlier attempts exceeded CPU limits |

The router forwarded between wired endpoints on opposite sides of the Wi-Fi uplink. Earlier two-Wi-Fi-hop and router-endpoint measurements are separate and must not be substituted for this topology. No test establishes a throughput ceiling, zero packet loss over time, ordinary-disturbance recovery, or PhotonSharp production readiness. Only one usable CPU was observed.

## Source and reproducibility

- `source/release-b43/`: exact b797 effective b43 source.
- `source/release-mac80211/`: exact CDC effective mac80211 source; it was built separately and is not the older mac80211 source found in the b797 build tree.
- `source/backports-support/`: matching compatibility/API source snapshot.
- `source/installed-b43/`: **historical original installed baseline**, not the newly selected release driver.
- `patches/release/`: baseline-to-qualified review diffs and individual page-pool/observer patches. Do not apply overlapping diffs cumulatively.
- `configs/release-module-options.conf`: exact qualified options.
- `tests/`: focused offline fixtures; these do not establish noncoherent ARM hardware correctness.

Exact module SHA256 identities:

```text
b43.ko      b7970aed0163bd050a7514c7f92803fff9379ce93f7d7b696cb449db84048789
mac80211.ko cdcf3316e0220fba0939e7274ae399bfe8c7dbb3deb4ad5c6faac8ee933ba172
```

See [build scope](docs/BUILD.md), [provenance/licenses](docs/PROVENANCE.md), [testing](docs/TESTING.md), [known issues](docs/KNOWN-ISSUES.md) and [recovery](docs/RECOVERY.md). A matching kernel version alone is insufficient for module compatibility. This is not an official OpenWrt-supported release.
