# Qualification evidence

The latest b797/CDC qualification ran for840.39seconds in RAM, with the router forwarding traffic between a Cox-side wired Mac and an Archer-LAN wired Linux host. Single-stream10second receive bursts stepped through10,25,50,100,150 and200Mbps. Upload bursts used10,25 and50Mbps. Quiet periods, existing SSH heartbeats, LuCI checks, DNS and low-rate connectivity probes continued. Eleven application SHA checks passed, including one during150Mbps traffic.

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
