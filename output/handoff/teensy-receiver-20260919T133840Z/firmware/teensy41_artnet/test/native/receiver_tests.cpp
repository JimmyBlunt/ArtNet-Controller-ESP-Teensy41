#include "receiver_core.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

uint8_t packet[530], output[artnet::kBytes];
void prepare(unsigned u,unsigned seq=1,unsigned len=510,uint8_t value=7) {
  memset(packet,value,sizeof(packet)); memcpy(packet,"Art-Net\0",8);
  packet[8]=0; packet[9]=0x50; packet[10]=0; packet[11]=14;
  packet[12]=seq; packet[13]=0; packet[14]=u&255; packet[15]=u>>8;
  packet[16]=len>>8; packet[17]=len&255;
}
void frame(artnet::Receiver& r,unsigned seq,uint32_t now,uint8_t value=7,int skip=-1) {
  for(unsigned u=120;u<=148;++u) if(u!=138 && int(u)!=skip) {
    prepare(u,seq,510,value); r.ingest(packet,528,now);
  }
}
int main() {
  unsigned cases=0;
  { artnet::Receiver r; frame(r,1,1); assert(r.counts.complete==1);
    assert(r.take(output)); for(auto c:output) assert(c==7); assert(!r.take(output)); ++cases; }
  { artnet::Receiver r; frame(r,1,1,7,127); assert(!r.ready());
    frame(r,2,34,9); assert(r.counts.incomplete==1); assert(r.take(output));
    for(auto c:output) assert(c==9); ++cases; }
  { artnet::Receiver r; frame(r,255,1); frame(r,1,34); assert(r.counts.complete==2);
    prepare(120,255); r.ingest(packet,528,35); assert(r.counts.stale==1); ++cases; }
  { artnet::Receiver r; frame(r,1,1); frame(r,1,34); assert(r.counts.complete==1);
    assert(r.counts.duplicate==28); ++cases; }
  { artnet::Receiver r; frame(r,0,1,8,123); frame(r,0,34,9);
    assert(r.counts.complete==1 && r.counts.incomplete==1); r.take(output);
    for(auto c:output) assert(c==9); ++cases; }
  { artnet::Receiver r; prepare(120); r.ingest(packet,528,0xfffffff0);
    r.expire(0x80); assert(r.counts.incomplete==1); ++cases; }
  { artnet::Receiver r; frame(r,1,1,10); frame(r,2,34,11);
    assert(r.counts.overwritten==1); r.take(output); for(auto c:output) assert(c==11); ++cases; }
  { artnet::Receiver r; prepare(120); packet[0]=0; r.ingest(packet,528,1);
    prepare(120); r.ingest(packet,20,1); prepare(120,1,511); r.ingest(packet,529,1);
    prepare(120,1,508); r.ingest(packet,526,1); assert(r.counts.rejected==4); ++cases; }
  { artnet::Receiver r; prepare(138); r.ingest(packet,528,1); assert(r.counts.ignored==1);
    prepare(32768); r.ingest(packet,528,1); assert(r.counts.rejected==1); ++cases; }
  { artnet::Receiver r; // Distinct universe payloads verify routing and physical split.
    for(unsigned u=120;u<=148;++u) if(u!=138) { prepare(u,1,512,u); r.ingest(packet,530,1); }
    assert(r.take(output));
    for(auto port:artnet::ports) for(unsigned p=0;p<port.pixels;++p)
      for(unsigned c=0;c<3;++c) assert(output[(port.offset+p)*3+c]==port.universe+p/170);
    ++cases; }
  { artnet::Receiver r; frame(r,1,1); r.clear(); assert(!r.ready());
    frame(r,1,2); assert(r.counts.complete==2); ++cases; }
  { artnet::Receiver r; frame(r,100,1); r.expire(1102); frame(r,1,1103);
    assert(r.counts.complete==2); ++cases; }
  for(unsigned firstSeq=0;firstSeq<2;++firstSeq) {
    artnet::Receiver r; prepare(120,firstSeq,510,7); r.ingest(packet,528,1);
    const unsigned nextSeq=1-firstSeq;
    for(unsigned u=121;u<=148;++u) if(u!=138) {
      prepare(u,nextSeq,510,9); r.ingest(packet,528,2);
    }
    assert(!r.ready()); prepare(120,nextSeq,510,9); r.ingest(packet,528,3);
    assert(r.take(output)); for(auto c:output) assert(c==9); ++cases;
  }
  { artnet::Receiver r; frame(r,100,1);
    prepare(120,101); r.ingest(packet,528,500);
    prepare(121,102); r.ingest(packet,528,1200); r.expire(1500);
    prepare(122,101); r.ingest(packet,528,1501); assert(r.counts.stale==1); ++cases; }
  artnet::Receiver r;
  const auto begin=clock();
  for(unsigned i=0;i<10000;++i) { frame(r,1+i%255,i*33); r.take(output); }
  const auto us=(clock()-begin)*1000000LL/CLOCKS_PER_SEC;
  assert(r.counts.complete==10000 && r.counts.incomplete==0);
  printf("{\"test_cases\":%u,\"stress_frames\":10000,\"complete\":%u,"
         "\"host_elapsed_us\":%lld,\"hardware_fps_measured\":false}\n",cases,r.counts.complete,(long long)us);
}
