# Known limitations

- A persistent unicast uplink stall occurred in earlier receive-load experiments while aggregate IRQ/RX counters continued. The incomplete-record/reorder-state explanation remains a hypothesis; short clean trials do not prove elimination.
- An offline TX completion-validation defect is separate from the receive-record guard. Its correction is not silently promoted here; hardware occurrence and causal relation to the stall remain unproven.
- Page-pool/DMA lifetime and cache-coherency behavior need broader hardware exposure. Host sanitizers and ARM userspace tests do not prove DMA correctness.
- 225 Mbps is an earlier short RAM result. Stable 250 Mbps was not qualified. No gigabit wireless-forwarding claim is made.
- The 14-minute qualification did not force disconnection, validate ordinary reconnection, perform an extended soak, or establish long-term hosting availability.
- Wi-Fi retries, failed transmission attempts and miscellaneous RX drops were nonzero; application integrity and sampled reachability passed. Station counters are not exact test-flow packet-loss counts.
- The source archive is not yet a clean, turnkey full-firmware build or a public generic installer. Private lab firmware is not suitable for distribution.
- Proprietary Broadcom microcode784.2 remains a radio dependency; the host driver is open source.
