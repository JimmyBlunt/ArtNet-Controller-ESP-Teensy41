#pragma once
#include <cstddef>
namespace fake {
inline bool pending = false;
inline bool storageAvailable = true;
inline bool writable = true;
inline int writes = 0;
}
class Preferences {
 public:
  bool begin(const char*, bool) { return fake::storageAvailable; }
  bool getBool(const char*, bool) { return fake::pending; }
  size_t putBool(const char*, bool value) {
    if (!fake::writable) return 0;
    fake::pending = value;
    ++fake::writes;
    return 1;
  }
  void end() {}
};
