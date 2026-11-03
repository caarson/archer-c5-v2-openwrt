# Archer C5 v2: OpenWrt and BCM4352 AC-PHY

Community engineering source release for **TP-Link Archer C5 v2**, OpenWrt **25.12.6**, Linux **6.12.112**, and a modified **b43** host driver based on [Alessio Ferri’s b43-ac-wip](https://github.com/aleferri/b43-ac-wip).

**Working experimental version — v0.1.1-618 (prerelease).** The owner selected the 618 build following a successful bounded 2.4 GHz diagnostic campaign: 870 seconds active, 878.24 seconds including cleanup, 23/23 transfers and 175 MB verified, plus 44/44 DNS and HTTPS rounds. This is a functional test pass, not proof the historical intermittent failure is fixed. The permanent configuration leaves the optional recorder disabled; the RAM qualification enabled it. See [working-version scope and installation](docs/WORKING-v0.1.1-618.md).

**Prerelease / device-specific research.** The b797 baseline and its BCM43217 2.4 GHz initialization correction have been installed persistently on the development router. Installation and cold-boot checks passed; see `evidence/permanent-installation.json` for b797 and `evidence/bcm43217-post-install.json` for the 2.4GHz follow-up. This repository provides matching driver source, review diffs, configurations, tests and measured results. It does not supply a generic flashable image or a stock-firmware web-upgrade path. Private provisioned images and proprietary radio microcode are excluded.

## What works in the lab

- BCM4352 5 GHz station uplink, with Internet forwarded to Gigabit Ethernet LAN clients.
- Existing Archer RF front-end adaptation, guarded deferred receive processing, RX/IP batching, RX synchronization correction and page-pool/bidirectional DMA support.
- Incomplete hardware A-MSDU record rejection and a separate mac80211 CCMP-IV guard. These are correctness measures; the historical persistent receive-load stall is **not proven fixed**.
- Essential fault monitoring retained; optional DMA timing disabled in the qualified configuration.

## Evidence, with limits

The table below describes the earlier **b797/CDC 5GHz qualification**. It is not a repeated qualification of the later4130 2.4GHz correction, whose result remains conditional.

| Exposure | Result |
|---|---|
| Earlier b797 bounded mixed-workload RAM qualification | PASS, 840.39 seconds (14 minutes) |
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

- `source/release-b43/`: b797 plus the targeted BCM43217 EPA initialization correction and optional bounded N-PHY recorder (default disabled).
- `source/release-mac80211/`: exact CDC effective mac80211 source; it was built separately and is not the older mac80211 source found in the b797 build tree.
- `source/backports-support/`: matching compatibility/API source snapshot.
- `source/installed-b43/`: **historical original installed baseline**, not the newly selected release driver.
- `patches/release/`: baseline-to-qualified review diffs and individual page-pool/observer patches. Do not apply overlapping diffs cumulatively.
- `configs/release-module-options.conf`: exact qualified options.
- `tests/`: focused offline fixtures; these do not establish noncoherent ARM hardware correctness.

Exact module SHA256 identities:

```text
b43.ko      618552caa7b515954d357f91273576ae716ff26b7e2fc4ef9fb51a82c0b0c7c0
mac80211.ko cdcf3316e0220fba0939e7274ae399bfe8c7dbb3deb4ad5c6faac8ee933ba172
```

See [build scope](docs/BUILD.md), [provenance/licenses](docs/PROVENANCE.md), [testing](docs/TESTING.md), [known issues](docs/KNOWN-ISSUES.md) and [recovery](docs/RECOVERY.md). A matching kernel version alone is insufficient for module compatibility. This is not an official OpenWrt-supported release.

## 2.4 GHz follow-up

Main now includes the [BCM43217 external-PA initialization correction](docs/BCM43217-24GHZ.md). WPA2-CCMP association, DHCP, two-way file checks and internet use have been demonstrated. This remains legacy802.11b/g, with no new throughput claim. The channel11 follow-up remains conditional: intermittent packet loss and occasional upload timeouts are unresolved. Unsuccessful diagnostic candidates are excluded; see the recorded post-install results. The `v0.1.0-b797` tag remains the previous baseline.

The latest 618 addition is diagnostics-only. Descriptor ownership, crypto, RF tables, queue behavior and recovery policy are unchanged from 4130. Prior unsuccessful fixes were not accumulated. The working designation means usable in the lab with known limitations; it does not certify public production hosting.
