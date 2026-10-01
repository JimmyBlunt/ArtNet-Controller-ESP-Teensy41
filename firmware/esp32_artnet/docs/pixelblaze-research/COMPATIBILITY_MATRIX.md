# Pixelblaze Compatibility Matrix

| Feature | Status | Evidence / Limit |
|---|---|---|
| 1D fallback coordinates | Implemented | `x = index / pixelCount`, `y = z = 0` behavior documented from public UI. |
| 2D coordinates | Implemented | Neutral JSON supports 2D points; tests cover missing dimensions. |
| 3D coordinates | Implemented | Neutral JSON supports 3D points and walled-cube example. |
| Fill | Implemented in tools/core | Normalization function and tests exist; exact Pixelblaze UI save behavior not written. |
| Contain | Implemented in tools/core | Normalization function and tests exist. |
| Pixelblaze coordinate array import/export | Implemented | `tools/mapping-utils.js#importPixelblazeArray` and `exportPixelblazeArray`; comments and trailing commas are tested. |
| Pixelblaze mapper generator preservation | Implemented offline | JavaScript mapper source is detected and preserved as non-executable metadata; it is not run by tools or firmware. |
| Pixelblaze script compiler | Not implemented by design | No arbitrary MCU JavaScript execution; large/generative maps must be resolved off-controller. |
| MariMapper CSV import/export | Implemented offline | `index,x,y,z` and `index,tx,ty,tz` are supported; missing indices are explicit placeholders. |
| Live Pixelblaze save observation | Not performed | Read-only safety constraint. |
| WebSocket binary PIXELMAP write | Documented only | Packet type observed; no write test. |
