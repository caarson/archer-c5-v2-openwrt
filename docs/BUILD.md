# Source preparation and build scope

This is a matching driver-source prerelease, not a tested turnkey full-firmware build/install recipe.

## Verified inputs

Use Linux for ARM cross-compilation. Recorded build tools were OpenWrt GCC 14.3.0, musl, soft-float ARM Cortex-A9, Linux 6.12.112 and backports 6.18.55. The exact build inputs and module identities are in `manifests/upstream-pins.json`. Kernel ABI/config, Module.symvers, all companion modules and firmware must match as a coherent set; a matching kernel version string alone is insufficient.

First verify this preview:

```sh
python3 tools/verify-preview.py
```

To inspect the original-layout integration against its exact OpenWrt base:

```sh
git clone --no-checkout https://github.com/openwrt/openwrt.git openwrt-review
git -C openwrt-review checkout --detach 64900e756efa667774214f9c6b17c113222e01f7
python3 tools/prepare-openwrt.py --source openwrt-review
```

The helper defaults to a check only. `--apply` applies the source patch to that clean development checkout; it does not run make, create a firmware image, access the router, or flash anything. Work in a separate source checkout.

## Important incompleteness

- The historical OpenWrt integration snapshot contains a packager with fixed historical **pre-AC** kernel/root payload hashes. It is retained as evidence of the original-layout format/validation, not a generic packager for the current AC image. Do not replace hashes merely to bypass validation.
- The kernel config embedded in that historical integration patch differs from the exact installed `configs/installed-kernel.config`. This discrepancy is explicitly retained and recorded; a future coherent build must resolve it before any deployment.
- `configs/openwrt-build.config` preserves recorded package selections but replaces the local external-toolchain root with `/opt/openwrt/toolchain-arm_cortex-a9_gcc-14.3.0_musl_eabi`. This normalization changes its hash and is not a byte-reproduction claim.
- The standalone backports b43 snapshot/patch series was built separately and integrated into a coherent lab root with 72 matched modules. Applying only a b43 patch to an arbitrary OpenWrt tree will not reproduce that root. The exact clean companion-module integration, AC microcode extraction/configuration and userspace packaging still require a public reproducible recipe.
- The historical six-patch series is not a substitute for this integration work. The BCMA/SSB patch touches bus/SPROM interfaces and must not be applied indiscriminately to the wrong kernel/backports tree.

The actual earlier developer commands included `make C5_ORIGINAL_LAYOUT=1 target/linux/compile`, then package compilation/install and independent kernel/root audits. They ran in a specially prepared tree with external toolchain/host corrections. They are historical context, not sufficient public release instructions.

## Binary release prerequisites

1. Resolve integration/config drift and reproduce a clean matching kernel, modules, radio dependency and userspace from pinned public inputs.
2. Generate generic provisioning: no lab authorized keys, saved wireless credentials, private network configuration, or device backups.
3. Review licenses and provide matching corresponding source/build scripts for the distributed binaries.
4. Independently validate original partition names/bounds, protected factory regions and the accepted upgrade path.
5. Qualify the resulting artifact and independent recovery on the exact device revision.

No step in this preview deploys or modifies a router.

## Preparation checks performed

The integration patch passed a check/apply/reverse-check/reverse round trip against the exact base contents, and the helper refused an unpinned checkout. Git reported 11 whitespace warnings in nested patch text; these historical inputs were retained rather than rewritten. The receipt is `evidence/source-preparation-check.json`. These checks do not compile the firmware or qualify it on hardware.

## Qualified b797/CDC components

The new `source/release-b43/` snapshot replaces the b43 directory in the matching configured backports6.18.55 tree. The `source/release-mac80211/` snapshot comes from a different pinned build tree and replaces `net/mac80211`; do not assume the mac80211 files in the b797 tree produced the CDC binary. `source/backports-support/` contains the effective include, compatibility and backport-include API files. Preserve their relative locations when reviewing integration. `configs/release-backports.config` and `configs/release-module-options.conf` record the build selections and runtime parameters.

Recorded module build shape, using an already prepared matching kernel tree and ARM toolchain:

```sh
make -j2 -C backports ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
  KLIB_BUILD="$MATCHING_KERNEL_BUILD" KLIB=/lib/modules/6.12.112 modules
"${CROSS_COMPILE}strip" --strip-unneeded backports/drivers/net/wireless/broadcom/b43/b43.ko
"${CROSS_COMPILE}strip" --strip-unneeded backports/net/mac80211/mac80211.ko
```

The recorded builds reused Linux6.12.112 configuration SHA256`04c42b16194e4814d6d2812f226ae4df934ed773b584251450341e2323e241d2` and Module.symvers SHA256`eb13c9705c7e789070e1d13c3bee63d90daa35d2719c52f82610735aa7d32513`. GCC14.3.0/musl ARM Cortex-A9 produced the qualified modules. Those commands describe the prepared build; a clean public full-image rebuild is still a prerequisite for a generic binary release. Do not insert these modules into an arbitrary running router.
