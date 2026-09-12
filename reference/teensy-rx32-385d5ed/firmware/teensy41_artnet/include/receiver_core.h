#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace artnet {
constexpr unsigned kPixels = 4105;
constexpr unsigned kBytes = kPixels * 3;
constexpr unsigned kPorts = 7;
struct Port { uint16_t universe, pixels, offset; };
constexpr Port ports[kPorts] = {
    {120,203,0}, {122,738,203}, {127,880,941}, {133,810,1821},
    {139,352,2631}, {142,680,2983}, {146,442,3663}};
constexpr uint32_t kExpectedMask = 0x1fffffffUL & ~(1UL << 18); // U138 unused
struct Counters {
  uint32_t packets=0, rejected=0, ignored=0, complete=0, incomplete=0;
  uint32_t stale=0, duplicate=0, overwritten=0;
};

// Allocation-free assembly. A frame is published only after all 28 universes.
// Sequence 0 is accepted for legacy senders, but cannot prove frame coherence.
class Receiver {
 public:
  Counters counts;
  bool ingest(const uint8_t* p, size_t n, uint32_t now) {
    ++counts.packets;
    if (n < 18 || memcmp(p,"Art-Net\0",8) || p[8]!=0 || p[9]!=0x50 ||
        (unsigned(p[10])*256+p[11])<14) { ++counts.rejected; return false; }
    const unsigned len=unsigned(p[16])*256+p[17];
    if (len<2 || len>512 || (len&1) || n!=18+len || (p[15]&0x80)) {
      ++counts.rejected; return false;
    }
    const unsigned u=p[14]+unsigned(p[15])*256;
    unsigned dest=0, copy=0;
    bool mapped=false;
    for (auto port:ports) {
      if (u>=port.universe && (u-port.universe)*170<port.pixels) {
        const unsigned pixel=(u-port.universe)*170;
        copy=(port.pixels-pixel)*3; if(copy>510) copy=510;
        dest=(port.offset+pixel)*3; mapped=true; break;
      }
    }
    if(!mapped) { ++counts.ignored; return false; }
    if(len<copy) { ++counts.rejected; return false; }
    expire(now);
    const uint8_t seq=p[12];
    if(seq) {
      if(!haveSequence_ && mask_) abandon(); // Legacy -> sequenced is a new generation.
      if(haveSequence_ && seq!=sequence_) {
        const unsigned delta=(unsigned(seq)+255-sequence_)%255;
        if(delta>127) { ++counts.stale; return false; }
        abandon();
      } else if(haveSequence_ && seq==sequence_ && sealed_) {
        ++counts.duplicate; return false;
      }
      if(!haveSequence_ || seq!=sequence_) {
        sequence_=seq; haveSequence_=true; sealed_=false;
      }
    } else {
      if(haveSequence_) abandon(); // Sequenced -> legacy must not retain old pixels.
      haveSequence_=false; sealed_=false;
    }
    const uint32_t bit=1UL<<(u-120);
    if(mask_&bit) {
      ++counts.duplicate;
      if(seq) return false;
      abandon(); // Legacy repeated universe starts a new candidate, never stale reuse.
    }
    if(!mask_) started_=now;
    memcpy(staging_+dest,p+18,copy); mask_|=bit;
    lastAccepted_=now;
    if(mask_!=kExpectedMask) return false;
    if(ready_) ++counts.overwritten;
    memcpy(complete_,staging_,kBytes);
    mask_=0; ready_=true; sealed_=bool(seq); ++counts.complete;
    lastComplete_=now;
    return true;
  }
  bool take(uint8_t* target) {
    if(!ready_) return false;
    memcpy(target,complete_,kBytes); ready_=false; return true;
  }
  void expire(uint32_t now) {
    if(mask_ && uint32_t(now-started_)>100) abandon();
    // Permit a restarted sender to re-establish its sequence after a long pause.
    if(uint32_t(now-lastAccepted_)>1000) { haveSequence_=false; sealed_=false; }
  }
  void clear() { mask_=0; ready_=false; haveSequence_=false; sealed_=false; }
  bool ready() const { return ready_; }
  uint32_t lastComplete() const { return lastComplete_; }
 private:
  void abandon() { if(mask_) ++counts.incomplete; mask_=0; }
  uint8_t staging_[kBytes]={}, complete_[kBytes]={};
  uint32_t mask_=0, started_=0, lastComplete_=0, lastAccepted_=0;
  uint8_t sequence_=0;
  bool ready_=false, haveSequence_=false, sealed_=false;
};
} // namespace artnet
