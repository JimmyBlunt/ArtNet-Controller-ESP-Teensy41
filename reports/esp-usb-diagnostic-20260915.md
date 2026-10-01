# ESP USB diagnostic — 2026-09-15

- User confirms ESP has only USB connected, no other wiring or loads.
- Windows COM6: CP210x, VID:PID 10C4:EA60, USB serial 0001.
- Passive 115200-baud capture repeatedly reports `Brownout detector was triggered`
  followed by `rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`.
  No controller application status appeared during the 12-second capture.
- esptool 4.11.0 chip identification and RAM stub execution succeeded.
- Chip: ESP32-D0WD revision 1.0, MAC e8:68:e7:0d:38:a4.
- Separate flash identification succeeded: manufacturer 20, device 4016,
  detected capacity 4 MB. These are identification checks, not a flash integrity test.
- Identity matches the older ESP .251 in Schrank-LED/STATUS.md (2026-09-10),
  where brownout on both older and updated application firmware was documented.
- No firmware or NVS erased/written. esptool reset the board after each check.

Interpretation: CPU, bootloader and flash identification respond. Complete board
failure is not established. Repeated brownout indicates a supply problem detected
on the board; USB cable/port/hub versus board regulator or other board fault is
not isolated. Next step: compare a known-good USB data cable and a different
adequately powered port before a firmware write. No brownout protection disabled.

Reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/fatal-errors.html#brownout

## Follow-up after USB connection change

- COM6 remains CP210x serial 0001; USB location changed from 1-6.4 to 1-4.4.
- Passive 15-second capture: no output. Controlled RTS reset and 25-second capture:
  one normal ROM/bootloader trace, no brownout and no controller application status.
- Probes of previously used addresses 10.0.0.251 and 10.0.0.248 did not respond.
- Identity freshly reconfirmed during backup: e8:68:e7:0d:38:a4, ESP32-D0WD rev1.0.
- Full 4-MB flash read completed at 460800 baud in 99.9 seconds without errors.
  Private recovery copy: C:\Temp\esp-recovery-20260915\before-flash-4mb.bin.
  SHA256: d7fa82340a6657bdbd66d24d10ad7b4bbc9910e30ad75d3df9983f92e27574a9.
  This backup can contain WiFi credentials; do not publish or commit the binary.
- Extracted old app0 image passed esptool checksum and validation hash checks.
  This verifies stored image integrity, not runtime or power stability.
- Current flexible-output profile and network boot guard native tests passed.
- Preparing current `esp32-wifi-flex8` firmware: it retains support for GPIO32 and
  APA102 GPIO18/19 from the older .251 setup; the newer restricted extension-board
  profile would reject those saved settings.

## Controller recovery and requested Orbital Prism update

- First `esp32-wifi-flex8` application built successfully: RAM 62,104 bytes;
  program 1,182,961 bytes. Firmware SHA256:
  04bf544d2dd2a11a0529fabfa3c5c32bd0b87899d7e9643de61ed95b8afceff9.
- USB write verified all four written regions. NVS was not erased.
- First application ran normally and passed a controlled restart: WiFi connected,
  IP 10.0.0.251 matched the identified MAC, safeMode=no, 25-second captures without
  brownout. API and serial confirmed saved configuration loaded: 731 WS2812B on
  GPIO14, 1024 APA102 on GPIO18/19, 1,755 pixels, 30 FPS, start universes 0/4.
- `/api/storage` reported saved=true, matches=false (string comparison with the
  newer serialized config); the existing saved settings were not overwritten.
- User then requested the Teensy background. Copied the exact 178,766-byte WebP
  into Schrank-LED/web/assets/orbital-prism.webp. SHA256:
  b4c8fa2e64357b95541509f430a0799301b410abf5d43823dba5e825d924be35.
- Added an SPIFFS-backed image endpoint and identical 50%-opacity positioning.
  Both existing desktop/embedded browser suites passed. Local browser verified
  image decode (1672x941), 0.5 opacity and no horizontal overflow at 1440 and 390 px.
  Screenshot: Schrank-LED/output/playwright/esp-orbital-local.png (local preview).
- Artwork build succeeded: RAM 62,136 bytes, program 1,221,349 bytes. Application
  file SHA256: 91123b5631453caa5db11e1f064e460113df4211df726ab647e6cb6707ee1aa1.
  SPIFFS image SHA256: aac008f62706407021c415c8c91b183275cb15235cd848c7ea01fe7124d41204.
- Application at 0x10000 and previously empty filesystem at 0x290000 both written
  and hash verified. Artifacts/logs: C:\Temp\esp-recovery-20260915\orbital.
- After this flash the device entered safeMode=yes. One controlled restart
  reproduced brownout immediately after BEFORE WiFi.mode, before web-server and
  artwork filesystem startup. It returned to safeMode=yes, IP 0.0.0.0, retaining
  the same two outputs and pixel counts. No repeated recovery attempts or
  brownout-protection bypass performed.
- Current state: artwork firmware installed, radio off in recovery. Live HTTP
  image verification is blocked by the recurring supply fault; earlier successful
  operation does not establish stable hardware. LED electrical/optical output
  remains untested (USB only). A physical power/cable/board comparison is needed.
