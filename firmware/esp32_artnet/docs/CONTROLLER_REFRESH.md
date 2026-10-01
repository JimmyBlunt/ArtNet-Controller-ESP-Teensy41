# Controller refresh — 2026-09-07

## Shared interface

`web/index.html`, `web/styles.css` and `web/app.js` are the source for both the local
desktop page and the ESP-served page. `node tools/build-web-ui.js` generates
`firmware/include/WebUi.generated.h`; PlatformIO runs it before compilation.
`node tools/build-web-ui.js --check` verifies that the generated file is current.
No separate hand-maintained HTML remains in WebApi.cpp.

The interface has Overview, Outputs, Mapping, Diagnostics and System pages. The
desktop page connects to an entered address; the embedded page uses its own origin.
No example outputs or online indicators are presented as live controller state.
The browser polls status and test state serially with timeouts and preserves dirty
fields. Output pins, color order and SPI rate are fixed by the supported profile.

## Output control

- `POST /api/test-pattern {"action":"start","outputId":1}` tests one enabled output.
- `{"action":"loop"}` tests both outputs repeatedly.
- `{"action":"stop"}` schedules one physical black frame, then releases Art-Net.
- `{"action":"blackout"}` schedules black and retains ownership until stopped or a
  test starts. It is not an electrical power disconnect.
- Tests do not start automatically on boot unless LED_HARDWARE_TEST_AUTOSTART is set.
- Art-Net is drained during tests/blackout so queued packets do not accumulate.

FastLED registers the actual configured lengths (512 and 1167 by default). Runtime
reconfiguration blacks the previously registered length before shortening or disabling
an output. Output routing copies directly from the logical buffer into fixed hardware
buffers, preserving reverse order without allocating per-output vectors each frame.

## Configuration persistence

- `POST /api/config` applies a validated configuration in RAM.
- `POST /api/config/save` stores the current applied configuration in ESP32 NVS.
- `GET /api/storage` reports `saved` and whether it `matches` the running configuration.
- The 1679 WiFi profile restores validated `config-v1` data on boot; invalid data falls
  back to compiled defaults. WiFi secrets are not stored in this configuration blob.
- Preview target, test state and blackout are temporary. Saving does not store them.

## Art-Net behavior and limits

Each universe occupies a fixed 170-RGB-pixel slot (510 channels). Short payloads leave
black padding and do not shift later universes. Sequence ordering is tracked per universe
with 255-to-1 wrap; zero disables sequence checking. Partial collections expire after
100 ms; a new packet for an already collected universe abandons an incomplete collection.
Receive bursts are bounded and output rendering uses the latest complete collection,
limited by target FPS. This is best-effort collection, not guaranteed sender-frame
synchronization: ArtSync and multi-sender merging are not implemented.

Protocol reference: [Artistic Licence Art-Net specification](https://art-net.org.uk/downloads/art-net.pdf).
Sequence values must not be treated as a global frame ID shared across universes.

`lastFrameLatencyMs` is null because end-to-end latency is not measured. FPS and output
time are software observations, not a measurement of the physical LED signal.

Mapping conversion remains PC-side. `POST /api/mappings` returns 501 instead of claiming
that uploaded coordinates affect LED output. Processing UDP preview remains available;
the browser's output overview explicitly is not a live pixel preview.

## Verification

Native regression tests cover stop/held blackout, output selection, fixed universe
offsets, timeout recovery, duplicate/out-of-order sequences and timer rollover.
The same browser test suite is exercised against file-based and embedded UI variants,
with mocked APIs: loading real-shaped controller config, typing during polling,
apply/save/reload, rejected edits, blackout, preview target, disconnection/recovery,
and mobile overflow. Screenshots in `output/playwright/*fixture.png` contain test data.

Hardware outcomes are recorded separately in STATUS.md. Mocked persistence tests do
not by themselves prove persistence across an actual ESP reboot.

## Isolated brownout diagnostic

The explicit `esp32-power-diagnostic` environment builds only `PowerDiagnostic.cpp`.
It uses the same Arduino/ESP32 framework but no LED driver, controller API or config
storage. It should log eight seconds of radio-off baseline before calling WiFi.mode,
then WiFi.begin. It does not print credentials or change the brownout protection.
It temporarily replaces the application; restore `esp32-wifi-ws2812-apa102-1679`
after measurement. It is excluded from normal firmware and native tests by its macro.

On the new board `d4:e9:f4:84:b5:cc`, the diagnostic upload was verified, but the
25-second monitor showed repeated brownout resets without reaching its first
application marker. A separate raw boot capture showed the same resets. Thus LED
drivers and the controller UI are not necessary to reproduce this failure. The test
does not identify whether the cause is the USB supply path, board regulator, or another
hardware fault; independent supply/board comparison is still required.

Reference: [Espressif brownout documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/fatal-errors.html#brownout).
