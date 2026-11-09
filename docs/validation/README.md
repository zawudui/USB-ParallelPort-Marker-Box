# Validation status

The evidence below is from one prototype board and one bench session on 2026-11-09.

| Check | Observation | Limitation |
|---|---|---|
| USB command/ACK | 100/100 transactions passed; 0.1796–0.3522 ms write-to-complete-ACK round trip | Host round trip, not one-way trigger latency |
| DB25 D1 pulse | One capture, approximately 2.002 ms high | Single event; not a statistical pulse-width test |
| MCU D1 to DB25 D1 | Approximately 31 ns edge difference in one capture | Probe/channel skew not calibrated; ground clips temporarily bridged isolated and host grounds during this empty-DB25 bench measurement |

Use appropriate differential/isolated measurement methods when measuring across the isolation barrier; ordinary earth-referenced probe grounds can defeat that barrier.

The raw acquisition files and local machine logs are intentionally not included in this initial public snapshot. A later evidence release may include a sanitized, independently reviewed dataset and reproducibility scripts.
