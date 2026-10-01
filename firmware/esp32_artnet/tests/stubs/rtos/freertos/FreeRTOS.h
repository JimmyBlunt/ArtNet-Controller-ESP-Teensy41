#pragma once
#include <cstddef>
#include <cstdint>
using BaseType_t = int;
using TickType_t = uint32_t;
constexpr BaseType_t pdTRUE = 1;
constexpr BaseType_t pdFALSE = 0;
struct StaticQueue_t {
  uint8_t* storage = nullptr;
  size_t capacity = 0;
  size_t itemSize = 0;
  size_t head = 0;
  size_t size = 0;
};
using QueueHandle_t = StaticQueue_t*;
