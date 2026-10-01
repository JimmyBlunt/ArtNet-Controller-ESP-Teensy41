#pragma once
#include "FreeRTOS.h"
#include <cassert>
#include <cstring>

// Host-only, single-threaded model of FreeRTOS's fixed-size copying queue.
// Tests cover our handoff policy, not FreeRTOS scheduling or cross-core locking.
inline QueueHandle_t xQueueCreateStatic(size_t capacity, size_t itemSize,
                                        uint8_t* storage, StaticQueue_t* control) {
  *control = {storage, capacity, itemSize, 0, 0};
  return control;
}
inline BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t wait) {
  assert(wait == 0);
  if (q->size == q->capacity) return pdFALSE;
  const size_t tail = (q->head + q->size) % q->capacity;
  std::memcpy(q->storage + tail * q->itemSize, item, q->itemSize);
  ++q->size;
  return pdTRUE;
}
inline BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t wait) {
  assert(wait == 0);
  if (!q->size) return pdFALSE;
  std::memcpy(item, q->storage + q->head * q->itemSize, q->itemSize);
  q->head = (q->head + 1) % q->capacity;
  --q->size;
  return pdTRUE;
}
