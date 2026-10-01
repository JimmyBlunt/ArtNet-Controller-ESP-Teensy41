#pragma once
#include <cstdint>
#include <string>
namespace fake {
inline uint32_t now = 0;
inline std::string input;
inline std::string log;
}
struct FakeSerial {
  int available() const { return static_cast<int>(fake::input.size()); }
  char read() { char c = fake::input.front(); fake::input.erase(0, 1); return c; }
  void print(const char* value) { fake::log += value; }
  void println(const char* value = "") { fake::log += std::string(value) + "\n"; }
  template <typename... Args> void printf(const char*, Args...) {}
  void flush() {}
};
inline FakeSerial Serial;
inline uint32_t millis() { return fake::now; }
inline void delay(uint32_t ms) { fake::now += ms; }
