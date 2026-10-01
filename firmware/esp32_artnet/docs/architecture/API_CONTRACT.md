# API Contract

Minimum embedded endpoints:

| Method | Path | Status |
|---|---|---|
| GET | `/api/status` | Contract documented |
| GET | `/api/config` | Contract documented |
| POST | `/api/config` | Contract documented |
| GET | `/api/outputs` | Contract documented |
| POST | `/api/outputs` | Contract documented |
| GET | `/api/artnet/stats` | Contract documented |
| GET | `/api/performance` | Contract documented |
| GET | `/api/mappings` | Contract documented |
| POST | `/api/mappings` | Contract documented |
| POST | `/api/test-pattern` | Contract documented |
| POST | `/api/reboot` | Contract documented |

The local web UI currently uses static mock data and the shared calculation module. Firmware HTTP handlers are planned and must be hardware/network tested before being marked complete.
