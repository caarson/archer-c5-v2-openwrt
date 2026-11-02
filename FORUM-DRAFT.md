# Experimental Archer C5 v2 BCM4352 AC-PHY source release

Draft only; not posted. Repository: https://github.com/caarson/archer-c5-v2-openwrt

We adapted Alessio Ferri’s b43-ac-wip for an Archer C5 v2/OpenWrt25.12.6 development router and documented the exact kernel/module identities, front-end adaptation and bounded qualification. The latest b797 configuration passed a14-minute functional RAM test including200Mbps receive bursts and application integrity checks. Earlier225Mbps exposures were short;250Mbps is not qualified. The historical uplink-stall mechanism and long-term reliability remain open.

This is source, patches and evidence, not an official OpenWrt release or a public generic flashable image. Proprietary Broadcom microcode is required and is not redistributed. Please review the build and recovery limitations before attempting integration.
