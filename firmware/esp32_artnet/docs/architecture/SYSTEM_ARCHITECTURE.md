# System Architecture

Pipeline:

```text
Art-Net -> UniverseAssembler -> LogicalPixelBuffer -> MappingEngine -> OutputRouter
        -> APA102 / WS2812B outputs
        -> Preview stream
```

The important integration rule is that physical output and preview use the same final mapped buffer.

Implemented and tested locally:

- `LogicalPixelBuffer`
- `UniverseAssembler`
- `MappingEngine`
- `OutputRouter`
- `Performance` calculations
- `PreviewProtocol`
- default mixed configuration

Hardware-dependent:

- ESP32 Ethernet/WiFi bring-up
- FastLED APA102/WS2812B physical output
- ArtPollReply on real network hardware
- long-run 60 FPS validation
