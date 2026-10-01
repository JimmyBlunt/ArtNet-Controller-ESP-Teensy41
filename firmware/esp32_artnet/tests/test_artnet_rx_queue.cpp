#include "ArtNetRxQueue.h"
#include "ArtNetReceiver.h"
#include <array>
#include <cassert>
#include <iostream>
#ifdef PIO_UNIT_TESTING
#include <unity.h>
#else
#define TEST_ASSERT_TRUE(condition) assert(condition)
#endif

void setUp() {}
void tearDown() {}

void testOrderCapacityAndBufferOwnership() {
  led::ArtNetRxQueue queue;
  TEST_ASSERT_TRUE(queue.begin());
  std::array<uint8_t, 530> input{};
  for (size_t i = 0; i < 64; ++i) {
    input.fill(static_cast<uint8_t>(i));
    TEST_ASSERT_TRUE(queue.push(input.data(), input.size()));
  }
  input.fill(255); // Caller may reuse/overwrite its receive buffer immediately.
  TEST_ASSERT_TRUE(!queue.push(input.data(), input.size()));
  TEST_ASSERT_TRUE(queue.packets() == 65 && queue.drops() == 1);
  TEST_ASSERT_TRUE(queue.begin()); // Idempotent begin must not clear queued data.
  led::ArtNetDatagram packet;
  for (size_t i = 0; i < 64; ++i) {
    TEST_ASSERT_TRUE(queue.pop(packet));
    TEST_ASSERT_TRUE(packet.length == input.size());
    for (uint8_t byte : packet.bytes) TEST_ASSERT_TRUE(byte == i);
  }
  TEST_ASSERT_TRUE(!queue.pop(packet));
  // Repeated refill/drain exercises wraparound with mixed datagram lengths.
  for (size_t round = 0; round < 5; ++round) {
    for (size_t i = 0; i < 47; ++i) {
      input.fill(static_cast<uint8_t>(i + round));
      TEST_ASSERT_TRUE(queue.push(input.data(), i + 1));
    }
    for (size_t i = 0; i < 47; ++i) {
      TEST_ASSERT_TRUE(queue.pop(packet));
      TEST_ASSERT_TRUE(packet.length == i + 1);
      for (size_t j = 0; j < packet.length; ++j)
        TEST_ASSERT_TRUE(packet.bytes[j] == i + round);
    }
    TEST_ASSERT_TRUE(!queue.pop(packet));
  }
  TEST_ASSERT_TRUE(queue.drops() == 1);
}

void testOversizedAndMalformedDatagrams() {
  led::ArtNetRxQueue queue;
  TEST_ASSERT_TRUE(queue.begin());
  std::array<uint8_t, 531> input{};
  std::memcpy(input.data(), "Art-Net\0", 8);
  input[9] = 0x50;
  input[16] = 2; // A valid 512-channel ArtDmx prefix in an oversized packet.
  TEST_ASSERT_TRUE(!queue.push(input.data(), input.size()));
  TEST_ASSERT_TRUE(queue.oversized() == 1 && queue.drops() == 0);
  led::ArtNetDatagram packet;
  TEST_ASSERT_TRUE(!queue.pop(packet));
  TEST_ASSERT_TRUE(queue.push(nullptr, 0));
  TEST_ASSERT_TRUE(queue.push(input.data(), 17));
  TEST_ASSERT_TRUE(queue.push(input.data(), 530));
  led::UniversePacket dmx;
  size_t consumed = 0, parsed = 0;
  while (queue.pop(packet)) {
    ++consumed;
    if (led::ArtNetReceiver::parseArtDmx(packet.bytes, packet.length, dmx)) ++parsed;
  }
  TEST_ASSERT_TRUE(consumed == 3 && parsed == 1);
  TEST_ASSERT_TRUE(dmx.data.size() == 512);
  TEST_ASSERT_TRUE(queue.packets() == 4);
}

int main() {
#ifdef PIO_UNIT_TESTING
  UNITY_BEGIN();
  RUN_TEST(testOrderCapacityAndBufferOwnership);
  RUN_TEST(testOversizedAndMalformedDatagrams);
  return UNITY_END();
#else
  testOrderCapacityAndBufferOwnership();
  testOversizedAndMalformedDatagrams();
  std::cout << "Art-Net queue handoff tests passed\n";
#endif
}
