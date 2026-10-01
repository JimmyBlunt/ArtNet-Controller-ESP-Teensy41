# Benchmark Results

Local smoke benchmark:

```sh
make benchmark
```

Generated files:

- `reports/benchmark-smoke.json`
- `reports/benchmark-smoke.csv`
- `reports/performance-matrix.json`
- `reports/performance-matrix.csv`
- `reports/hardware-benchmark-esp32-wifi-mixed-4500-30fps-3s.json`

## Evidence Classes

| Class | Files | Meaning |
|---|---|---|
| Calculated | `reports/performance-matrix.*` | Formula-based LED timing and Art-Net bandwidth estimates. Not hardware measurements. |
| Simulated sender | `reports/benchmark-smoke.*` | Local packet generation report. It proves tool execution and expected packet counts, not controller reception. |
| Measured controller API | `reports/hardware-benchmark-esp32-wifi-mixed-4500-30fps-3s.json` | Real ESP32 WiFi controller counters before/after a UDP Art-Net run, read through HTTP API. |

## Measured ESP32 WiFi API Run

Date: 2026-08-03

Target: ESP32 WiFi controller at `10.0.0.246`

Scenario: `mixed_4500_30`, 4,500 pixels, 27 universes, 30 FPS target, 3 seconds.

Result from `reports/hardware-benchmark-esp32-wifi-mixed-4500-30fps-3s.json`:

| Metric | Value |
|---|---:|
| Sent packets | 2,430 |
| Received packet delta from controller API | 646 |
| Estimated lost packets | 1,784 |
| Complete frame delta | 2 |
| Incomplete frame delta | 0 |
| Completion ratio | 0.0222 |
| Reported packets per second after run | 199 |
| Heap free after run | 192,452 bytes |
| Minimum heap free during run | 151,316 bytes |
| HTTP `/api/status` latency before run | 1,347.84 ms |
| HTTP `/api/status` latency after run | 77.06 ms |
| HTTP `/api/performance` latency after run | 66.88 ms |

Interpretation: ESP32 WiFi did not keep up with the 4,500 pixel / 27 universe / 30 FPS mixed load in this short test. This is a measured controller/API result, not a physical LED-output verification.

## Still Not Measured

- APA102 signal timing on a logic analyzer or real strips.
- WS2812B signal timing on a logic analyzer or real strips.
- CPU load, because the current firmware API does not expose CPU utilization.
- Frame latency and LED output time in a real run after flashing the newer API build. The source now exposes `frameTimeUs`, `outputTimeUs`, and `lastFrameLatencyMs`, but the saved WiFi benchmark report was captured before that API extension was flashed and rerun.
- RMII Ethernet, W5500, and Teensy 4.1 runtime measurements, because those hardware paths were not connected and exercised.
- Processing long-run FPS under sustained controller traffic.
