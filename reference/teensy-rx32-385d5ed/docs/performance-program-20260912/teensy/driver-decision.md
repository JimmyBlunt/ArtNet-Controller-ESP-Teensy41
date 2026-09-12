# FastLED-Treiberentscheidung, 2026-09-12

Der aktuelle Firmwarestand verwendet die öffentliche Channel-API aus dem gepinnten FastLED-Release 3.10.4. `FastLED.setExclusiveDriver<fl::Bus::OBJECT_FLED>()` registriert gezielt den modernen ObjectFLED-Engine. Sieben `ChannelConfig` mit `ClocklessChipset`, explizitem Bus, GRB, Gamma 1 und deaktiviertem Dithering werden über `Channel::create` und `FastLED.add(channel)` registriert. Das vermeidet die Registrierung unbenötigter Plattformtreiber durch `FastLED.add(config)`.

Dies bleibt FastLED mit dem integrierten ObjectFLED-DMA-Backend. Es verwendet weder OctoWS2811 noch ein Octo-Board. Es handelt sich um eine API-Auswahl, nicht einen geratenen Buildschalter. `FASTLED_USES_OBJECTFLED` allein beziehungsweise `addLeds<WS2812B,...>` führte in diesem Release zum älteren Proxy.

## Geprüfte Primärquellen

Alle Pfade relativ zum unveränderten [FastLED-Tag 3.10.4](https://github.com/FastLED/FastLED/tree/3.10.4):

- `src/FastLED.h`: öffentliche `setExclusiveDriver<Bus>()`, `add(ChannelPtr)` und `add(ChannelConfig)`.
- `src/fl/channels/config.h`, `channel.h`, `options.h`: Konfiguration und öffentliche Channel-Erzeugung.
- `examples/Sailboat/Sailboat.ino`: offizielles Beispiel mit `ClocklessChipset` und `ChannelConfig`.
- `src/platforms/arm/teensy/teensy4_common/drivers/objectfled/bus_traits.h`: Registrierung des tatsächlichen `ChannelEngineObjectFLED`.
- `channel_engine_objectfled.cpp.hpp` im selben Ordner: sammelt alle Strip-Metadaten vor `onQueuingDone`, kopiert anschließend alle Payloads und ruft `instance->show()` auf.
- `objectfled_peripheral_real.cpp.hpp`: setzt bereits `drawBuffer=frameBufferLocal`.
- `src/fl/channels/manager.cpp.hpp`: Frame-End-Ereignis ruft den Engine auf.

## Warum der erste Versuch ersetzt wurde

Der erste Legacy-Build scheiterte beim Boot. Ein eng begrenzter SHA-gesicherter Zeigerpatch und die Verschiebung der Initialisierung auf ARM machten USB/Ethernet/ARM wieder verfügbar, aber der erste Mehrkanaltransfer scheiterte weiterhin. Ein unabhängiger zweiter Agent bestätigte drei Quellpfade:

1. Der Legacy-Wrapper ließ den von `show()` gelesenen `drawBuffer` null. Der erste Patch behob dies; die tatsächliche Pointerzuweisung wurde im ELF nachgewiesen.
2. Er finalisierte das Rechtecklayout bereits nach dem ersten Strip. Weitere Strips erhielten dadurch leere Zielspannen.
3. Sein Frame-End-Proxy rief keinen Flush auf; `Registry::flushAll()` hatte keinen Aufrufer.

Diese Legacyfehler sind im modernen Engine bereits vermieden. Deshalb kein immer breiterer lokaler Vendorpatch. Der Build-Hook stellt ausschließlich unseren exakt bekannten früheren Einfügetext wieder auf den SHA-geprüften Originaltext zurück. Historischer Patch, Tests und gescheiterte Images bleiben als Diagnosebeleg erhalten.

## Grenzen und weitere Prüfung

Die moderne Engine kommentiert die Ausgabe als synchron, aber der konkrete ObjectFLED-Aufruf startet DMA asynchron. Unsere Anwendung prüft deshalb weiterhin den gepinnten DMA-Manager, bevor sie den geteilten Transferbuffer erneut überschreibt. Vor dem allerersten Transfer wird die noch nicht initialisierte DMA-DONE-Flag nicht als Sperre interpretiert.

Das geprüfte Profil ist ausschließlich RGB, sieben feste Pins, eine WS2812-Timinggruppe. Keine allgemeine RGBW- oder dynamische Rekonfigurationsfreigabe. Die erste Hardwareausgabe und die Dauerleistung dieses modernen Pfades werden gesondert gemessen; erfolgreiche Quellprüfung allein belegt keine 30 FPS.
