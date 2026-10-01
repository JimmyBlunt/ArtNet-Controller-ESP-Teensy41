# Art-Net data flow, frame integrity and performance

Updated 2 October 2026. This guide describes the current Teensy web build
`teensy41_octo_web_rx32`, compares the ESP assembler where relevant, and explains
archived hardware measurements. It does not describe the historical Teensy
receiver as if it were the active web implementation.

## Protocol boundary

ArtDmx transports one universe of channel values over UDP port 6454. Its header
is 18 bytes, followed by an even payload of 2–512 bytes. The header contains the
`Art-Net\0` identifier, little-endian opcode `0x5000`, big-endian protocol version,
sequence, physical port, 15-bit Port-Address and big-endian payload length.
Sequence values 1–255 support ordering; zero disables that mechanism. ArtSync
can synchronize buffered universe output across nodes. See the official
[Art-Net 4 specification, revision 1.4dp, ArtDmx and ArtSync sections](https://art-net.org.uk/downloads/art-net.pdf).

In this project, “frame” means the assembled RGB image required by configured
outputs. It is not another network packet, nor does a high UDP packet count
prove a complete image. The 510-channel packing is our 170-RGB-pixels-per-universe
layout; ArtDmx itself permits up to 512 channels.

Neither current receive path implements ArtSync. The minimal Teensy web receiver
accepts ArtDmx only and does not implement ArtPoll discovery or sender merging.
The recorded tests therefore use a manually configured unicast destination.
This is an application-specific receive path, not a claim of full Art-Net support.

## Dependencies and ownership

| Stage | Teensy 4.1 / Octo | ESP32 |
|---|---|---|
| Network | Native Ethernet, QNEthernet; RX32 driver patch | Wi-Fi and ArtNetReceiver |
| Assembly | `runtime_artnet::Receiver`, global route mask | `UniverseAssembler`, sparse active-universe set |
| Sequence state | Shared across all routes; no sender identity | Per universe |
| Output | FastLED Channels / ObjectFLED, parallel WS2812B lanes | FastLED output routing, WS2812B / APA102 by profile |
| Configuration | Web + EEPROM | Web + NVS |
| Image asset | Embedded Orbital Prism WebP | Orbital Prism in SPIFFS |
| Physical adapter | Level shifting and two LED RJ45 connectors | Depends on selected board profile |

The browser changes configuration and commands tests; it is not in the LED data
transport path. A preview displays software intent, not measured light. Teensy
uses separate staging, complete and output buffers so partial receive data is
not deliberately submitted as a new complete frame.

## Teensy: from a datagram to a complete image

The implementation is [runtime_receiver.h](../firmware/teensy41_artnet/include/runtime_receiver.h),
especially `configure`, `ingest`, `expire` and `abandon`.

1. **Count arrival.** `packets` increments before validation. Rejected and ignored
   traffic therefore still contributes to this counter.
2. **Validate.** Require the identifier, ArtDmx opcode, protocol version at least
   14, valid address high bit, even length 2–512 and an exact datagram size of
   `18 + declared length`.
3. **Find a route.** Each enabled port contributes `ceil(pixelCount / 170)` routes.
   Unknown universes increment `ignored`. There is no fixed range 120–152.
4. **Require enough RGB bytes.** A route requires the smaller of 510 bytes or the
   remaining pixel count times three. Extra payload bytes are not mapped to LEDs.
5. **Apply sequence policy.** See the table below. Only accepted route payloads
   update staging data and set their route bits.
6. **Publish only when all bits are present.** One dynamically built mask covers
   every active route across the controller, up to 64 routes. Completion copies
   staging data into the ready buffer and increments `complete`.
7. **Keep the newest complete image.** If an older complete image is still waiting,
   it is replaced and `overwritten` increments. This is distinct from an incomplete
   candidate or a lost network packet.

### Expected universes and short final payloads

| Output | Default pixels | Required universe range | Last route RGB bytes | Smallest even payload |
|---|---:|---|---:|---:|
| 1 | 203 | 120–121 | 99 | 100 |
| 2 | 738 | 122–126 | 174 | 174 |
| 3 | 880 | 127–132 | 90 | 90 |
| 4 | 810 | 133–137 | 390 | 390 |
| 5 | 352 | 139–141 | 36 | 36 |
| 6 | 536 | 142–145 | 78 | 78 |
| 7 | 512 | 146–149 | 6 | 6 |
| 8 | Disabled | None | — | — |

The default is **4,031 pixels and 29 universes**: 120–137 plus 139–149. Universe
138 is a gap; a stored but disabled OUT8 allocation at 150–152 contributes no
expected routes. Universe numbers here are the numeric wire addresses, so check
whether a sender UI labels them with a one-based offset.

The [19 September device snapshot](ARTNET_RECEIVER.md) had **610 pixels on OUT6**,
not the default 536. That still needs four universes, but U145 requires **300
bytes** instead of 78. A sender transmitting the default tail can therefore
deliver plenty of packets while the saved configuration rejects that route.
This is a concrete mismatch to check, not proof of the cause of any current fault.

### Sequence and incomplete candidates

| Event | Current Teensy behavior |
|---|---|
| Non-zero sequence | All routes of a candidate must share one sequence value |
| Newer sequence | Abandon any non-empty partial candidate; begin the new sequence |
| Old sequence | Modulo-255 delta greater than 127 is stale; reject it |
| Wrap | 255 → 1 is supported |
| Duplicate route, same non-zero sequence | Count duplicate, ignore; completed sequence is sealed |
| Sequence zero | Disable ordering; a repeated route abandons the partial candidate and starts collecting again |
| Switch zero / non-zero mode | Abandon a non-empty partial candidate |
| Candidate age strictly greater than 100 ms | Abandon partial data; timer starts at the first accepted route, not the last |
| More than 1,000 ms since an accepted packet | Reset sequence tracking |

`incomplete` counts abandonment of a **non-empty candidate**, not every missing
universe and not every interval without data. An administrative `clear()` does
not increment it. Since no sender identity is used, multiple sources can interfere;
use one coordinated source for this receive contract.

The ESP [UniverseAssembler](../firmware/esp32_artnet/firmware/src/UniverseAssembler.cpp)
checks sequence per universe and expires partial collection at **at least 100 ms**.
It can compose short data with black padding. Its semantics must not be inferred
from the stricter Teensy route-length and shared-sequence rules. Sequence zero
provides weaker temporal coherence; a full route set alone does not prove all
values were generated at the same instant.

## When LED output happens

See [web_main.cpp](../firmware/teensy41_artnet/src/web_main.cpp), particularly
`timing`, `transferReady`, `renderArtNet` and `loop`.

For ordinary Art-Net rendering, the node must be running/armed, the run policy
must permit rendering, the previous DMA transfer must be ready, the frame period
must be due and a complete frame must be available. Tests and blackouts are
separate output paths. A successful arrival does not directly call `show()`.

The timing calculation is:

```text
wire_guard_us = 30 × longest_enabled_lane_pixels + 300
output_period_us = max(ceil(1,000,000 / target_fps), wire_guard_us)
```

The transfer-ready check observes DMA completion, applies its latch wait and
wire guard, and updates a software completion counter. `FastLED.show()` call time
includes synchronous preparation; it is not the whole asynchronous transfer time.
Neither value is an optical sensor measurement. At 880 pixels the nominal guard
is 26.70 ms; a 30-FPS target requests a period of 33,334 µs.

Valid configuration boots with Art-Net ON. Link loss or more than 1,000 ms without
a complete frame triggers one blackout while preserving the armed state; fresh
complete data allows automatic recovery. Local RGB port tests support one/all
active outputs and single/loop operation, followed by restoration of the prior
state. See [the web implementation notes](OCTO_WEB.md).

## Performance and bottlenecks

The evidence is the [13 September hardware report](../reports/ethernet-matrix-20260913/BERICHT_DE.md)
and its [CSV](../reports/ethernet-matrix-20260913/ergebnisse.csv). There are 42 short
load cases across three series plus 13 intentional missing-universe/recovery
checks. Validation of report consistency does **not** mean every performance case
passed. The main series had complete reception in 18 of 25 cases.

One particularly useful recorded case used the 4,031-pixel map, a 40-FPS sender
target and a 30-FPS output target:

| Measurement | Recorded result |
|---|---:|
| Duration | 30.019342 s |
| Packets sent / received | 34,655 / 34,655 |
| Complete frames | 1,195 (39.808/s) |
| Submits / DMA completions | 900 / 900 (29.981/s) |
| Complete waiting frames replaced | 295 |
| Incomplete candidates / UDP queue drops | 0 / 0 |

For this settled run, **1,195 = 900 + 295**. The output scheduler intentionally
kept the latest complete frame. This identity is not a universal instantaneous
counter invariant: pending frames, in-flight output, tests, blackouts and resets
must be accounted for when comparing other windows.

| Constraint | What it limits | Evidence / interpretation | Controlled next step |
|---|---|---|---|
| Output target | Maximum requested output cadence | 40 input / 30 output explains replacement above | Match target to source within measured wire/CPU budget |
| Longest lane | Parallel LED transfer time | 900 pixels → 27.3 ms guard; 1,000 → 30.3 ms | Balance or shorten chains; allow software margin |
| Packet bursts | Time available to service receive buffers | 32 RX descriptors, UDP queue 96; loop drains up to 128 packets | Measure ring drops and service gaps; compare paced sending |
| CPU preparation | Delays before network service resumes | Copying and synchronous `show()` work coexist with DMA | Profile before changing copies, clearing or lookup logic |
| Sender scheduling | Actual cadence and burst shape | Windows achieved rates differ from targets | Log missed deadlines; avoid catch-up bursts |
| Evidence quality | What a counter can establish | Black data and one-second telemetry hide pixel corruption and fine jitter | Frame IDs/CRC, timestamps, non-black patterns and optical checks |

At 29 universes × 30 FPS, the nominal source produces **870 ArtDmx packets/s**.
An observed 890 packets/s does not establish the right universe coverage, payloads
or sequence. At 48 universes × 40 FPS, full 510-byte payloads plus the 18-byte
ArtDmx header total about **8.11 Mbit/s**, excluding UDP/IP/Ethernet overhead.
Average bandwidth alone therefore does not describe burst tolerance.

The pacing series tested 100 µs interpacket spacing. It did not remove all
problems for longer profiles. A receive-ring overflow before the UDP queue is a
plausible explanation for missing packets with zero UDP queue drops, but host,
NIC and switch loss remain possible. A larger RX ring is an **unmeasured candidate**,
not a verified fix; check memory/alignment costs and measure hardware drop counters.
The existing [full report](../reports/ethernet-matrix-20260913/BERICHT_DE.md)
records case-by-case outcomes and hypotheses.

These were roughly 30-second black-payload cases from a Windows unicast sender,
not a TouchDesigner qualification or long-duration optical/power test. The data
establish observed software counters, not byte-perfect displayed images or a
guaranteed maximum frame rate.

## A practical diagnostic and tuning loop

1. Record firmware identity, saved mapping, sender settings and target FPS.
   Compare the saved configuration with the source defaults.
2. Measure packet, rejected, ignored, stale, duplicate, incomplete, complete,
   replacement, submit and DMA deltas over one consistent interval.
3. If packets advance but complete frames do not, inspect wire addresses, tail
   lengths and sequence first. A capture identifies which required route fails.
4. If complete frames advance but output does not, inspect armed/test state,
   link/run policy, DMA readiness and output-period gating.
5. If output advances slower than reception, separate deliberate replacements
   from receive loss. Check target FPS and the longest-lane budget.
6. Change one variable: source cadence, pacing, port balance or an instrumented
   receive-buffer variant. Repeat the baseline and compare the same metrics.
7. Omit a required universe deliberately, confirm no new frame is submitted,
   then restore a complete frame and verify recovery. Extend to repeated,
   longer runs and non-black port/pixel patterns before hardware acceptance.

This documentation adds no firmware instrumentation and performs no new hardware
test. CRC checks, richer timing histograms and alternative ring sizes above are
proposals to evaluate, not features silently added to the current build.
