#include "KickAlignment.h"
#include <iostream>
#include <cstdlib>
#include <memory>
using namespace phasetwin;
void check(bool ok,const char* name){if(!ok){std::cerr<<"FAIL "<<name<<'\n';std::exit(1);}std::cout<<"PASS "<<name<<'\n';}
int main(){
 KickAnalyzer analyzer;auto storage=std::make_unique<KickData>();auto& data=*storage;data.count=kickCaptureSize;
 for(int hit=1;hit<8;++hit)for(int n=0;n<1500;++n){int i=hit*3000+n;if(i>=data.count)break;float x=float(std::sin(2*3.141592653589793*60*n/kickRate)*std::exp(-double(n)/300));data.audio[2][i]=data.audio[3][i]=x;if(i+24<data.count)data.audio[0][i+24]=data.audio[1][i+24]=-x;}
 auto r=analyzer.analyse(data,{});std::cout<<"hits="<<r.hits<<" lag="<<r.lag<<" confidence="<<r.confidence<<" gain="<<r.improvement<<'\n';check(r.reliable,"consistent hits accepted");check(r.inverted,"polarity recovered");check(std::abs(r.lag-24)<1,"timing recovered");check(r.after>r.before+.1,"interaction improves");
 KickConfig timingOnly;timingOnly.allowPolarity=false;auto timingResult=analyzer.analyse(data,timingOnly);check(!timingResult.inverted,"timing-only preserves polarity");
 KickConfig polarityOnly;polarityOnly.allowTiming=false;polarityOnly.currentLag=24;auto polarityResult=analyzer.analyse(data,polarityOnly);check(std::abs(polarityResult.lag-24)<1e-9,"polarity-only preserves timing");check(polarityResult.reliable && polarityResult.inverted,"preserve timing still learns useful polarity");
 KickConfig heldPolarity;heldPolarity.currentInverted=true;heldPolarity.allowPolarity=false;auto heldResult=analyzer.analyse(data,heldPolarity);check(heldResult.reliable && heldResult.inverted && std::abs(heldResult.lag-24)<1,"preserve polarity still learns useful timing");
 KickConfig c;c.allowTiming=false;c.allowPolarity=false;r=analyzer.analyse(data,c);check(r.lag==0 && !r.inverted,"measure only preserves correction");
 auto silentStorage=std::make_unique<KickData>();auto& silence=*silentStorage;silence.count=kickCaptureSize;check(!analyzer.analyse(silence,{}).reliable,"silence rejected");
 for(int i=0;i<data.count;++i){int hit=(i-24)/3000;if(hit%2==0)data.audio[0][i]=data.audio[1][i]=-data.audio[0][i];}
 check(!analyzer.analyse(data,{}).reliable,"conflicting held-out kick hits rejected");
 data.audio[0][200]=std::numeric_limits<float>::quiet_NaN();check(!analyzer.analyse(data,{}).reliable,"nonfinite rejected");
 KickCapture capture;capture.prepare(48000,180);auto capturedStorage=std::make_unique<KickData>();auto& captured=*capturedStorage;for(int i=0;i<192000;++i)capture.push(captured,{0,0},{0,0});check(captured.count==kickCaptureSize,"four second capture duration");
}
