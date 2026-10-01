#include "../firmware/teensy41_artnet/include/runtime_receiver.h"
#include <cstdio>
#include <vector>
using namespace runtime_artnet;
const Port ports[8]={{120,203},{122,738},{127,880},{133,810},{139,352},{142,610},{146,512},{150,0}};
void packet(Receiver& r,unsigned u,unsigned len,unsigned seq,unsigned t){
 std::vector<uint8_t> p(18+len,0); memcpy(p.data(),"Art-Net\0",8);p[9]=0x50;p[11]=14;p[12]=seq;p[14]=u&255;p[15]=u>>8;p[16]=len>>8;p[17]=len&255;r.ingest(p.data(),p.size(),t);
}
int main(){
 bool ok=true;
 for(int scenario=0;scenario<6;++scenario){
  Receiver r;r.configure(ports);unsigned count=0;
  for(auto port:ports)for(unsigned u=0;u<(port.pixels+169U)/170U;++u){
   unsigned length=(port.pixels-u*170)*3;if(length>510)length=510;length=(length+1)&~1U;
   unsigned universe=port.universe+u;
   if(scenario==1&&universe==145)length=78;
   if(scenario==2&&universe==149)continue;
   unsigned seq=scenario==3?count+1:scenario==5?0:1;
   packet(r,universe,length,seq,10+count++);
  }
  if(scenario==4)for(unsigned u:{138,150,151,152})packet(r,u,510,1,45);
  r.expire(200);
  const char* names[]={"shared_sequence_valid_lengths","old_OUT6_length_78","missing_universe_149","sequence_incremented_per_packet","extra_unused_universes","sequence_zero"};
  printf("%s: complete=%u rejected=%u ignored=%u incomplete=%u duplicate=%u\n",names[scenario],r.counts.complete,r.counts.rejected,r.counts.ignored,r.counts.incomplete,r.counts.duplicate);
  bool complete=scenario==0||scenario==4||scenario==5;
  ok &= r.counts.complete==(complete?1U:0U);
  if(scenario==1)ok &= r.counts.rejected==1;
  if(scenario==4)ok &= r.counts.ignored==4;
 }
 return ok?0:1;
}
