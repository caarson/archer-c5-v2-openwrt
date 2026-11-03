# Qualification evidence

The earlier b797/CDC qualification ran for840.39seconds in RAM, with the router forwarding traffic between a Cox-side wired Mac and an Archer-LAN wired Linux host. Single-stream10second receive bursts stepped through10,25,50,100,150 and200Mbps. Upload bursts used10,25 and50Mbps. Quiet periods, existing SSH heartbeats, LuCI checks, DNS and low-rate connectivity probes continued. Eleven application SHA checks passed, including one during150Mbps traffic.

Peak interval/safety-gate CPU was86.34%; minimum available memory64604KiB. Essential fault counters remained zero. One incomplete hardware-record event was seen and dropped; this does not prove the historical stall’s cause or cure. Sampled wired, upstream and public echo probes all received replies; gateway/public timings were router-originated, not full rack-to-Internet latency.

See `evidence/b797-short-qualification.json`. Older `installed-qualification.json` describes the original baseline, and `ram-research.csv`/the250Mbps aborted-trial evidence describe distinct experiments. Do not combine their averages or exposure durations.

Offline fixtures can be inspected/compiled separately:

```sh
cc -std=c11 -O1 -g -fsanitize=address,undefined tests/test_rx_pool.c -o /tmp/test_rx_pool
/tmp/test_rx_pool
cc -std=c11 -O1 -g -fsanitize=address,undefined tests/test_trigger_metadata.c -o /tmp/test_trigger_metadata
/tmp/test_trigger_metadata
python3 tools/verify-preview.py
```

These are models/focused fixtures, not a substitute for the effective kernel implementation or real DMA. A successful build does not establish hardware safety. No benchmark or deployment starts automatically from this repository.

## 618 working-version functional exposure

The 2.4 GHz diagnostics-only RAM build completed 870 seconds of active monitoring and 878.24 seconds including cleanup. Twenty USB-client transfers (2 and 4 Mbps paced, alternating directions) and three Mac uploads (2 Mbps) verified 175 MB total. All 44 DNS and 44 HTTPS rounds passed. Peak sampled CPU was 37.42%, minimum available RAM was 59,668 KiB, with no observed association reset, persistent stall or critical safety event. No throughput ceiling is established by paced traffic.

Local USB-to-AP ping median was 1.95 ms and p95 9.82 ms. All 869 observed sequence numbers were answered; the final in-flight interval is censored because no terminal summary was captured. Nonzero retries/failed transmissions and historical intermittent timeouts remain concerns. A separate four-flow alternating Mac exposure also completed, but must not be added to the 870-second duration as though it were one campaign. These are not long-term reliability or public-hosting certifications.

The RAM exposure enabled a target recorder. The selected permanent configuration uses the same exact module with that optional recorder disabled. Permanent reboot/functionality checks are separate evidence, not a repeat of the 15-minute profile. See `evidence/working-version-short-qualification.json` and `evidence/working-version-installation.json`.

Additional header-parser fixture:

```sh
cc -Wall -Wextra -Werror -fsanitize=address,undefined tests/test_nphy_trace_parser.c -o /tmp/c5-nphy-parser
/tmp/c5-nphy-parser
```

This checks truncated/malformed input and header-only bounds; it does not prove kernel concurrency, RF, crypto or DMA hardware correctness.
