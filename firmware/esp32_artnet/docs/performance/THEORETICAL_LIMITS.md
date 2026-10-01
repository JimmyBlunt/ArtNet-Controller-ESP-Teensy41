# Theoretical Limits

These are calculations, not measured hardware results.

WS2812B estimate:

```text
frameTimeUs = pixelCount * 30 + resetTimeUs
```

APA102 estimate:

```text
frameTimeUs = (32 start bits + pixelCount * 32 data bits + ceil(pixelCount / 2) end bits) / spiHz
```

Art-Net estimate:

```text
universes = ceil(pixelCount / 170)
dataRateMbps = (530 bytes * universes * fps * 8) / 1,000,000
```

Warnings are raised when estimated output time exceeds the configured target frame period.
