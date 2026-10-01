#pragma once

#include "Types.h"

namespace led {

inline bool validColorOrder(ColorOrder order) {
  return static_cast<unsigned>(order) <= static_cast<unsigned>(ColorOrder::BGR);
}

// Express the requested wire bytes in the existing controller's input order.
// This allows live changes without registering another FastLED controller.
// APA102 controllers transmit BGR; WS2812B controllers transmit GRB.
inline Rgb colorForFixedDriver(Rgb color, ColorOrder order, OutputType type) {
  Rgb wire;
  switch (order) {
    case ColorOrder::RGB: wire = color; break;
    case ColorOrder::RBG: wire = {color.r, color.b, color.g}; break;
    case ColorOrder::GRB: wire = {color.g, color.r, color.b}; break;
    case ColorOrder::GBR: wire = {color.g, color.b, color.r}; break;
    case ColorOrder::BRG: wire = {color.b, color.r, color.g}; break;
    case ColorOrder::BGR: wire = {color.b, color.g, color.r}; break;
  }
  return type == OutputType::APA102 ? Rgb{wire.b, wire.g, wire.r}
                                    : Rgb{wire.g, wire.r, wire.b};
}

}  // namespace led
