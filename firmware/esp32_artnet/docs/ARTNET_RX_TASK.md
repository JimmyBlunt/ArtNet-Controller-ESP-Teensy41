# Art-Net receive task

The core-0 `artnet-rx` task (priority 2, 4096-byte stack) owns a blocking lwIP
UDP socket bound to port 6454. The receive timeout is 20 ms. Socket failures
are retried after 250 ms; task creation failures are retried by loop every second
without disabling the web API. `SO_RCVBUF` is requested only when compiled into
lwIP. It does not change the precompiled UDP receive mailbox size.

A statically allocated FreeRTOS queue holds 64 complete datagrams of up to
530 bytes (about 34 KiB). Receiving and queueing allocate no per-packet heap
storage. A full queue drops the new datagram, preserving FIFO order. A 531-byte
receive buffer detects oversized datagrams, which are rejected rather than
parsing their truncated prefix. Malformed datagrams do not stop queue draining.

Only loop parses ArtDmx and accesses UniverseAssembler, including expiry,
hardware-test discard, composition and statistics. It drains until the queue
is empty. Output timing, sequence rules, NVS, boot guard, OTA and the 4 MHz
APA102 rate are unchanged. Core-0 reception can continue while core 1 outputs
LEDs or handles HTTP; a long HTTP/OTA operation can still fill the finite queue.

Additional `/api/status` counters:

| Field | Meaning |
| --- | --- |
| `rxPackets` | UDP datagrams removed from the socket, including malformed/oversized/dropped datagrams |
| `rxQueueDrops` | Datagrams rejected because the application queue was full |
| `rxOversizedPackets` | Datagrams larger than 530 bytes, rejected whole |
| `rxSocketErrors` | Socket open/bind/option/receive errors, excluding receive timeout/interruption |

These atomic unsigned 32-bit counters are cumulative since boot, including
across runtime config changes, and wrap modulo 2^32. They are copied to status
on each loop iteration. The existing `packets`, `droppedPackets` and
`sequenceErrors` remain assembler counters. Loss before `recvfrom` cannot be
counted by these counters; compare the sender's transmitted packets to measure it.

`python tools/test_host.py` runs existing regression suites and queue tests;
`python -m platformio test -e native` runs the queue tests using a single-threaded
FreeRTOS queue model. Tests cover FIFO across wraparound, 64-slot saturation,
drop counting, buffer reuse, full 530-byte payloads, mixed datagram lengths,
oversized rejection, and malformed traffic followed by valid ArtDmx. These tests
do not emulate lwIP, FreeRTOS scheduling, or the physical LED output.

Before OTA, export `/api/config`, `/api/storage` and `/api/status` separately for
both hosts and hash the files. A config export is a restorable LED configuration,
not a whole-flash/NVS-partition dump. WiFi credentials remain in the ignored local
`firmware/include/WifiSecrets.h`. Restore LED settings using `POST /api/config`
with the saved JSON, then `POST /api/config/save`, and verify exact config equality
and `saved`/`matches` at `/api/storage`. Update `.248` with the extensionboard build
first; verify it before updating `.251` with the repaired esp251 build.
