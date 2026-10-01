# WiFi boot recovery

The ESP32 profile arms `led-boot/wifi-pending` in NVS before enabling its radio.
If startup is interrupted by a reset (including power loss), the next boot leaves
the radio off and reports `safeMode=yes`. Unlike RTC RAM, this marker survives a
power interruption. A normal completed startup, including a router connection
timeout, clears the marker. Storage failures fail closed, without starting the radio.
This guards the startup window only; it is not protection against every possible
runtime reset. A deliberate power-off during startup can also trigger recovery.
The marker is independent of the user's `artnet-led/config-v1` settings; no saved
LED configuration is erased for recovery. It adds two committed flag writes per
completed radio startup, not per frame or loop iteration.

## Recovery

Use COM6 at 115200 baud, 8-N-1, no flow control. Close other serial monitors before
flashing or running automated probes. Safe mode retains the normal controller
firmware and output configuration, but offers no network API or Art-Net reception.
LED tests do not auto-start in the normal 1679 build.

After correcting the supply/USB setup, send the exact line `wifi-retry` followed
by LF or CRLF. This performs one startup attempt, preserving the guard until the
attempt completes. A subsequent reset returns to safe mode. Do not repeatedly
clear or bypass the guard to mask a hardware fault. No brownout protection is disabled.

## Diagnostic environment

`esp32-power-diagnostic` is a separate temporary firmware, not the controller.
It starts without LED drivers or radio activation. Commands: `w` enables STA mode,
`c` connects using the ignored local credentials, `r` restarts. It never prints
credentials. Flash the normal profile again when diagnostics finish.

On 2026-09-07, board e0:5a:1b:6c:9c:e8 remained stable with radio off and lost USB
during `WiFi.mode(WIFI_STA)`, before connection was attempted. The same failure
occurred without FastLED or the web UI. This identifies the failing stage, not the
electrical root cause. Stable supply verification is still required. See
[Espressif troubleshooting](https://docs.espressif.com/projects/esptool/en/latest/esp32/troubleshooting.html#insufficient-power).

The boot guard's native regression test substitutes test-only Arduino/WiFi/NVS
implementations. It tests guard ordering, interrupted startup, explicit retry,
serial parsing, storage errors and timeout behavior; it does not simulate radio
hardware, validate real NVS electrically, or prove WiFi performance.

## Powered-hub comparison (2026-09-07)

The same board and unchanged controller firmware connected successfully after the
user moved USB to a hub with its own power supply (COM6, topology 1-4.4). One
`wifi-retry` completed, IP 10.0.0.248 matched MAC e0:5a:1b:6c:9c:e8, safeMode=no.
Two API-triggered reboots connected normally. A temporary 29-FPS configuration
survived save/reboot; original 30-FPS configuration was then restored, saved and
verified after another reboot. Three black Art-Net frames (30 packets) were received
and output with no incomplete frames or sequence errors. Optical LEDs and sustained
performance remain unverified. This comparison implicates the previous USB/power
path but is not an electrical measurement identifying a particular faulty component.
