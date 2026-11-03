# Provenance, licenses and attribution

## Pinned inputs

| Component | Identity |
|---|---|
| OpenWrt | 25.12.6, commit `64900e756efa667774214f9c6b17c113222e01f7` |
| Linux | 6.12.112 |
| Linux backports | 6.18.55 |
| b43 AC-PHY upstream | `aleferri/b43-ac-wip@7ae3a61e61f2cc7b7e84f5bba3918401ea46816e` |
| Original baseline b43 module SHA256 | `9c3a794391d13c458e55a34b61ce837be0c6a0d04d83fa8907c4ee0a05646d81` |
| Original baseline mac80211 module SHA256 | `163ff394d48bd61adc9f62b69fa839531d0a2579a0a69b0c69ada63ae794f3ce` |
| Installed kernel config SHA256 | `04c42b16194e4814d6d2812f226ae4df934ed773b584251450341e2323e241d2` |

The earlier control reconstruction produced a b43 module byte-identical to the installed baseline. The copied C/header/Kconfig/Makefile files have their own hashes in `manifests/source-files.json`. This is not a claim that the public preview already reproduces a complete installable firmware image.

## Patch origin

- `816-02-core-support-for-the-ac-phy.patch`, `816-03-acphy-bringup.patch`: imported AC-PHY/core bring-up work from the pinned upstream integration.
- `880-bcma-ssb-support-the-BCM4352-BCM4360-family.patch`: bus/SPROM family integration; its kernel and backports applicability must be reviewed against their respective trees.
- `816-04-refuse-unverified-frontends.patch`: local guard for front ends that were not qualified.
- `816-05-frontend3-sub0-5ghz.patch`: local Archer front-end path, using the documented R7000 reference capture and independently matching the Archer vendor object's table bytes. Its historical “on-air validation pending” comment is retained unchanged; later lab observations are documented separately.
- `816-06-arm-safe-5850us-busy-wait.patch`: local ARM delay/build correction.

The review comparison against the pinned upstream files is not a standalone replay series. Existing copyright notices remain authoritative; credit includes Alessio Ferri, Andreas Wendleder, Rafał Miłecki and the other b43/Linux contributors named in the files. This repository makes no claim to authorship of imported AC-PHY code.

## Licensing scope

Files retain their existing SPDX/copyright notices, including GPL-2.0-only and GPL-2.0-or-later notices. The pinned b43-ac-wip repository also carries a top-level GPLv3 license; its exact notice and upstream provenance document are retained in `third_party/`. Do not replace these with a blanket MIT license or assume every imported file has the same license. New helper scripts and project documentation are offered under GPL-2.0-or-later; pre-existing material remains under its own notices. License texts are in `LICENSES/`.

No proprietary radio blobs, reverse-engineering binary captures, vendor modules, device-specific calibration or factory/flash dumps are included. Firmware 784.2 is a proprietary dependency, not an open-source host driver. Its archive origin and extracted-file hashes are recorded in `manifests/upstream-pins.json`; no assertion of redistribution permission for that blob is made here. A binary firmware release requires its own component-license and corresponding-source review.

## Newly qualified release components

The selected release uses b43 SHA256`b7970aed0163bd050a7514c7f92803fff9379ce93f7d7b696cb449db84048789` and mac80211 SHA256`cdcf3316e0220fba0939e7274ae399bfe8c7dbb3deb4ad5c6faac8ee933ba172`. `manifests/release-source-files.json` freezes their separate effective source snapshots and support headers. Review deltas are relative to the byte-reproduced original baseline. Page pooling, bidirectional RX mapping, batching, deferred handoff and diagnostics are local changes; the incomplete hardware-A-MSDU guard restores the upstream check identified by commit`247b2fc4077da75d5145796edfc1f97e5df5f796`. RF tables, calibration and proprietary microcode remain unchanged. No optional TX observer or unsupported fast-TX capability is enabled.

The lab permanent image is provisioned privately and is not a release asset. Source publication does not assert that a generic public image has been built or qualified.

Disabled diagnostic code retains lab-local address/flow filters for source fidelity. These are not portable configuration and do not affect the qualified configuration with the corresponding optional observers disabled. The station address in the observer fixture is required to match that helper’s filter. No saved wireless keys, SSH keys, NVRAM/calibration data or household packet captures are published.

## 618 optional diagnostic delta

The selected working experimental source adds a locally authored bounded N-PHY header observer to the 4130 initialization correction. Only b43.h, xmit.c, sysfs.c and two new observer headers change. There is no new RF table, ownership, crypto-policy, rate-control or recovery change. Exact current identities are in `manifests/upstream-pins.json`; historical b797 and 4130 evidence is retained.
