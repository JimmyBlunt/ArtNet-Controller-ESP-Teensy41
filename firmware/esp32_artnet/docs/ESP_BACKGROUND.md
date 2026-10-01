# Orbital Prism background

The ESP web menu uses the same unmodified `orbital-prism.webp` artwork as the
Teensy Octo build, at 50% opacity and with the same left-arc alignment. The shared
CSS references `web/assets/orbital-prism.webp`; the ESP serves that file at
`/assets/orbital-prism.webp` from its SPIFFS data partition.

The 178,766-byte original does not fit alongside the application in its OTA slot.
It is therefore stored separately, streamed without loading the complete file
into a String, and cached by the browser. No external image service is needed.

## Installation and updates

For the connected classic ESP32, build `esp32-wifi-flex8`. Build the filesystem
image with `pio run -e esp32-wifi-flex8 -t buildfs` and upload it with the matching
`uploadfs` target after identifying the device. `platformio.ini` sets `data_dir`
to `web/assets`; the image contains only the artwork.

A filesystem upload replaces that partition: preserve any existing files first.
For the initial 2026-09-15 installation, the complete flash was backed up and the
entire SPIFFS partition was confirmed erased before writing it.

Normal `/update` application OTA updates retain the artwork partition. Replacing
the artwork itself requires a filesystem update. Keep the partition layout
unchanged. NVS configuration is in a separate partition.

The firmware mounts SPIFFS without automatic formatting. If the filesystem or
image is absent, the menu retains its CSS gradient background and the controller
continues operating; the image endpoint returns 404.

Source artwork: `ArtNet-Controller 5/firmware/teensy41_artnet/web/orbital-prism.webp`.

## Verification on 2026-09-15

Firmware and filesystem built and flashed to ESP32-D0WD rev1.0,
MAC e8:68:e7:0d:38:a4 (COM6), with both write hashes verified. Both existing
browser test suites passed. Local desktop/mobile previews decoded the original
1672x941 image, used opacity 0.5 and showed no horizontal overflow.

The device subsequently reported brownout during WiFi activation, before artwork
filesystem/web-server startup, and stayed in safe mode after one explicit retry.
Live device image delivery could not be verified. Saved outputs still loaded:
731 WS2812B on GPIO14 and 1024 APA102 on GPIO18/19. Supply diagnosis remains open.
