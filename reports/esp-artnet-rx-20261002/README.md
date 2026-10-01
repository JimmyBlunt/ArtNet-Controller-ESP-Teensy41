# ESP32 Art-Net RX implementation and device verification

Work session: 2026-10-02 Australia/Sydney (device/browser records dated 2026-10-01 UTC).
Base: `24e609b`. Firmware source: `4049b602cf9b8a04321a2ddeb6d26c4cd7b20758`.
Branch: `fix/esp32-artnet-rx-task`.

## Result and acceptance

Both ESP32s were updated over Wi-Fi OTA, with no USB connection or flash erase.
Their original LED configurations were backed up before writes and restored after
each temporary test. Both original hardware profiles and saved configurations
are required to match in the final verification record.

**The requested performance acceptance has NOT passed.** The implementation,
host tests, builds and OTA/configuration checks succeeded. The 30-second
measurement on `.251` did not meet 99.9% received packets or 59.5 complete fps.
An additional approximately 30-fps source continued sending to `.251` during
testing. A controlled single-source 60-fps acceptance test remains necessary.
Do not interpret the absence of application queue drops as absence of WLAN or
lwIP packet loss, or claim the extra source fully explains the remaining loss.

## Firmware changes

- `ArtNetReceiver.cpp/.h`: a core-0 task at priority 2 owns the blocking BSD UDP
  socket. It retries socket failures and passes datagrams through the queue.
- New `ArtNetRxQueue.h`: 64 statically allocated, copied datagrams of at most
  530 bytes; FIFO order, full-queue and oversized-packet counters. The receive
  buffer has one extra byte to detect oversized datagrams before parsing.
- `main.cpp`: drains the queue completely; parsing, configuration access,
  assembler, expiry and output remain on the loop task. Task startup can retry
  without disabling HTTP.
- `SystemStats.h` and `WebApi.cpp`: expose `rxPackets`, `rxQueueDrops`,
  `rxOversizedPackets`, `rxSocketErrors` in `/api/status`.
- `platformio.ini`: fixes the `esp32-wifi-esp251` profile inheritance to match
  the existing device's `flex8-ws2812-apa102` profile and configured pins.
- Queue/native/profile regression tests, host runner, build documentation and
  Makefile were updated. See `git show --stat 4049b60` for the exact file list.

APA102 remains at 4 MHz. No physical LED image-quality assessment was possible
remotely, so the optional SPI speed increase was not attempted. Wi-Fi sleep was
already disabled. NVS, OTA authentication, boot guard and assembler sequencing
were retained.

## Backup and configuration audit

Private local backup directory:
`C:\Users\jimmy\Documents\ESP-Backups\2026-10-02-before-artnet-rx-task`.

It contains per-device `/api/config`, `/api/storage`, `/api/status`, a SHA-256
manifest, browser exports, OTA records and `Restore-Config.ps1`.
This is a restorable LED configuration backup, **not a complete flash image or
raw NVS-partition backup**. Wi-Fi credentials remain in the ignored local header
and are absent from Git. Private firmware images also stay outside Git.

| Device | Original/final profile | Active outputs | Target |
| --- | --- | --- | --- |
| 10.0.0.248 | esp32-wroom-flex-8ws-2apa | Output 6 APA102, 136 LEDs, logical start 203, pins 18/5, BGR, U149 | 60 fps |
| 10.0.0.251 | flex8-ws2812-apa102 | APA102 256 LEDs at start 0, pins 18/19, RGB, U156; 549 LEDs at start 256, pins 25/26, RGB, U158 | 60 fps |

`.248` retains total pixelCount 339 and its six disabled outputs; `.251` retains
total pixelCount 805. Exact per-field originals are in the private JSON backups.

Observed write sequence and recovery:

1. Exported and hashed both configurations and status/storage before OTA.
2. Installed the extensionboard RX build on `.248` first.
3. A concurrently running task, “Portiere Controller auf Teensy 4.1”, then
   installed a generic flex8 build on `.248`. The profile mismatch caused default
   runtime settings, while the original NVS settings remained saved. That task
   confirmed its write and stopped further device/build activity. Reinstalling
   the correct extensionboard firmware recovered the original configuration;
   no USB recovery or NVS erase was needed.
4. Installed the matching RX build on `.251`; both configuration checks passed.
5. Browser loop, blackout and stop controls temporarily changed test mode.
   Stop returned both devices to Art-Net operation.
6. `.251` Apply/Save/Reload preserved its configuration. The same test on `.248`
   exposed the pre-existing UI normalization issue described below. The original
   configuration was immediately restored and saved, then checked exactly.
7. Browser export and reboot checks confirmed persistence on both devices.
8. For isolated test-frame accounting, all configured input universe starts were
   temporarily shifted by +1000, without saving NVS. They were restored in the
   script's `finally` block and `saved=true, matches=true` was checked for both.
9. A priority-17 receiver experiment was built and installed `.248` then `.251`.
   It did not demonstrate improvement, and the sender itself ran below target.
   The experiment was reverted to the committed priority-2 implementation.
   Trial binaries and configuration/OTA records are retained privately in
   `priority17-trial`; the trial measurement is included here for transparency.
10. Rebuilt the committed source and reinstalled `.248` then `.251`. Final binary
    hashes, configuration equality, saved-state and status checks are recorded
    in `final-verification.json` and `final-firmware-sha256.json`.

No permanent configuration changes are intended or accepted. The exact temporary
configuration JSONs and their restoration results are retained in the backup
directory, including `10.0.0.248-browser-roundtrip-changed.json`,
`priority2-config-changes.json`, and `priority17-trial/isolated-test-config-changes.json`.

## Automated checks and builds

- `python tools/test_host.py`: all 18 recorded compile/run/UI-generation checks
  passed. See `host-test-results.json` for commands and output.
- `python -m platformio test -e native`: two Unity cases passed, including queue
  handoff coverage. See `native-tests.log`.
- Both `esp32-wifi-extensionboard` and `esp32-wifi-esp251` built successfully.
  The initial build is recorded in `firmware-build.log` and `firmware-sha256.json`;
  the final rebuild is recorded separately in `final-firmware-build.log` and
  `final-firmware-sha256.json`.
- Queue tests cover ordering/wraparound, copied-buffer ownership, mixed lengths,
  capacity/full counters, oversized datagrams, and draining after malformed input.
  Hardware-profile regression tests accept `.251`'s existing 805-pixel layout.

Builds used a separate `C:\codex-build\esp-rx-ota-20261002` output directory to
avoid collisions with another task's builds. Native queue tests model FreeRTOS
copying semantics; they do not simulate actual scheduling or WLAN losses.

## Network measurements

The installed PXLBLZ router generated a rainbow at requested 60 fps. The route
layout emitted one packet/frame to `.248` and six to `.251`. Sender logs and raw
before/after controller counters are included beside this report.

The priority-2 30-second run used test universes +1000 to avoid mixing frame
assembly with the pre-existing sender. That sender still consumed network/CPU
capacity and its original-universe datagrams were rejected by the assembler.
The test therefore measures the selected frames under additional traffic, not
the requested single-source workload. Valid packet count is the delta of
`packets - droppedPackets`; it is compared against actual sender frame count
times each controller's universes per frame. No other source on the shifted
universes was observed, but packet capture was not available to prove this.

| Priority-2 run | .248 | .251 |
| --- | ---: | ---: |
| Expected test packets | 1,785 | 10,710 |
| Valid test packets | 1,785 | 10,610 |
| Received | 100% | 99.0663% |
| Complete frames / 30 s | 1,785 | 1,766 |
| Complete fps | 59.50 | 58.87 |
| Incomplete frames | 0 | 9 |
| Output fps (counter) | 59.00 | 58.73 |
| RX queue drops / socket errors | 0 / 0 | 0 / 0 |

Sender: 1,785 frames, reported 59.90 fps, zero send errors. Dividing counts by
the requested 30 seconds gives the conservative rates above; router startup and
its reported active-send interval differ. `.251` saw 7,228 rejected background
datagrams in this interval. A zero send-error count does not establish delivery.

`before-ota.json` records an observational baseline. `esp248-first-check.json`
records a shorter mixed-firmware check (.248 updated, .251 still original).
Neither is a controlled before/after comparison of both updated devices.

The priority-17 trial is in `priority17-isolated-30s.json` with its sender log.
It is not evidence for a causal priority comparison: workload/sender timing
varied. No claimed performance gain relies on this discarded experiment.

After reinstalling the exact committed priority-2 binaries, a final 10-second
receive smoke test (`final-receive-smoke-10s.json`) confirmed both receivers
operate, but also confirmed variable remaining loss: `.248` received all 599
test packets; `.251` received 3,500/3,594 (97.3845%), with 581 complete and four
incomplete frames. Queue/socket error deltas remained zero. This short test is
not the 30-second acceptance test and must not be omitted when assessing the
remaining problem. Runtime configurations were again restored exactly and
verified against saved NVS after the test.

## Browser verification (Playwright skill)

Real Chrome was driven with the Playwright CLI. Scripts and screenshots are in
`../../output/playwright/`; recordings and the browser trace are in the private
backup's `Browser` directory. The trace is kept private because update-page
responses may contain transient OTA tokens.

| Check | Result |
| --- | --- |
| Connect/status and configured outputs on both hosts | Passed |
| Loop, blackout, Stop → Art-Net | Passed; test-state API and output counters verified |
| `.251` Apply, Save, Reload | Exact configuration retained |
| `.248` Apply, Save, Reload | Existing normalization defect found; backup restored |
| Mapping / diagnosis / system navigation | Exercised with screenshots |
| Firmware update page | Opened successfully in browser; actual OTA performed by verified uploader |
| Configuration export on both | Export contents matched original backups |
| Reboot and reconnect on both | Original configuration and saved/matches retained |
| Mobile 390×844 on both | Screenshots reviewed; `.251` layout usable |
| Final firmware browser status smoke | See `browser-final-check.json` |

The existing `.248` UI calls `normalizeLayout` when reading form values.
Applying an unchanged form changed total pixelCount 339 to 136 and changed
logical starts from 203 to 0 for outputs 1–6. This behavior exists in the base
commit, not just this receiver change. It was not fixed in this firmware task.
Avoid Apply/Save on this sparse/disabled-output layout until the UI is corrected.

No JavaScript exception was observed in the checked flows. `.248` did report
missing optional `orbital-prism.webp` (and a favicon request); the page remained
usable with its fallback background. This is not a claim of an error-free console.
Visual LED colour, signal integrity and physical brightness remain unverified.

## Remaining acceptance work

Pause the independent `.251` sender, verify that counters stay idle, and repeat
the 30-second 60-fps rainbow test with the PC sustaining the target rate and only
one source. Require `.251` >=99.9% packets, >=59.5 complete fps, near-zero
incomplete frames, approximately 60 output fps; `.248` >=59 output fps with zero
packet loss. If loss persists, investigate loss before `recvfrom` (including the
SDK's six-entry UDP mailbox) with packet capture and controlled network load.
The present evidence does not justify declaring those acceptance criteria met.
