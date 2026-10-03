#include "ScopePreferences.h"
#include "ScopeHistory.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace phasetwin;
void check(bool pass,const char* name){if(!pass)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
int main(){try{
 auto defaults=ScopePreferences::unpack(ScopePreferences{}.packed());check(defaults.style==1 && defaults.mode==0 && defaults.division==5 && defaults.sync && defaults.showA && defaults.showB,"first-open defaults are explicit");
 for(int sum=0;sum<2;++sum)for(int style=0;style<2;++style)for(int mode=0;mode<2;++mode)for(int channel=0;channel<3;++channel)for(int division=1;division<=7;++division){ScopePreferences p;p.summed=sum!=0;p.style=style;p.mode=mode;p.channel=channel;p.division=division;p.sync=false;p.showA=false;p.milliseconds=4000;auto r=ScopePreferences::unpack(p.packed());if(r.summed!=(sum!=0) || r.style!=style || r.mode!=mode || r.channel!=channel || r.division!=division || r.sync || r.showA || !r.showB || r.milliseconds!=4000)throw std::runtime_error("view roundtrip");}
 check(true,"168 summed/style/mode/channel/division combinations roundtrip");
 ScopePreferences invalid;invalid.style=99;invalid.channel=-50;invalid.division=99;invalid.milliseconds=std::numeric_limits<double>::infinity();auto r=ScopePreferences::unpack(invalid.packed());check(r.style==1 && r.channel==0 && r.division==7 && r.milliseconds==125,"invalid settings safely bounded");
 r=ScopePreferences::unpack(0xffffffff);check(r.channel<=2 && r.division<=7 && r.milliseconds<=4000,"malformed packed preferences bounded");
 check(filledBounds(.2f,.7f)==std::make_pair(0.f,.7f),"positive fill reaches zero baseline");check(filledBounds(-.7f,-.2f)==std::make_pair(-.7f,0.f),"negative fill preserves signed lobe");check(filledBounds(-.8f,.6f)==std::make_pair(-.8f,.6f),"mixed envelope fill retains both peaks");
 ScopeHistory history;ScopePacket packet;for(int n=0;n<scopePacketSamples;++n)for(int ch=0;ch<2;++ch){const float a=n%2?-.8f:.8f;packet.traces[ch][n]=a;packet.traces[2+ch][n]=-a;packet.traces[4+ch][n]=-a;packet.traces[6+ch][n]=-a;}
 history.ingest(packet);check(history.range(4,0,0,256,256)==std::make_pair(0.f,0.f),"raw before sum preserves exact cancellation");check(history.range(5,0,0,256,256)==std::make_pair(-1.6f,1.6f),"raw corrected sum retains constructive amplitude");
 for(int k=1;k<80;++k){packet.firstSample=std::uint64_t(k*scopePacketSamples);history.ingest(packet);}const int count=history.availableSamples(4000);
 check(count>16384 && history.range(4,0,0,count,count)==std::make_pair(0.f,0.f),"long sum envelope preserves cancellation instead of adding independent extrema");check(history.range(5,2,0,count,count)==std::make_pair(-1.6f,1.6f),"long mono sum retains constructive peaks");
 ScopePreferences legacy;legacy.summed=false;auto legacyPacked=legacy.packed() & ~(1u<<22);check(!ScopePreferences::unpack(legacyPacked).summed,"older saved view remains stacked");
 ScopePreferences hidden;hidden.showA=hidden.showB=false;r=ScopePreferences::unpack(hidden.packed());check(!r.showA && !r.showB,"both hidden waves remain a valid visual state");
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
