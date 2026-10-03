#include "AudioEngine.h"
#include <iostream>
#include <stdexcept>
using namespace phasetwin;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);std::cout<<"PASS "<<text<<'\n';}
int main(){try{
 constexpr double pi=3.14159265358979323846;double worst=0;
 for(double frequency:{.01,.1,.25,.4,.45})for(double fraction:{.1,.37,.5,.9}){
  Delay delay;delay.prepare(512);double error=0,energy=0;const double requested=64+fraction;
  for(int n=0;n<5000;++n){delay.push(float(std::sin(2*pi*frequency*n)));auto actual=delay.readBandlimited(requested);double ideal=std::sin(2*pi*frequency*(n-requested));if(n>512){error+=(actual-ideal)*(actual-ideal);energy+=ideal*ideal;}}
  const double rms=std::sqrt(error/energy);worst=std::max(worst,rms);if(rms>=.001){std::cerr<<"frequency/sr "<<frequency<<" fraction "<<fraction<<" error "<<rms<<'\n';throw std::runtime_error("fractional delay exceeds -60 dB relative error");}
 }
 std::cout<<"worst relative RMS error="<<worst<<" ("<<20*std::log10(worst)<<" dB)\n";
 check(true,"20 frequency/fraction combinations below -60 dB error through 0.45 sample rate");
 for(int rate:{44100,48000,96000,192000}){const double bound=std::ceil(rate*.02);for(double lag:{-bound,-bound+.37,0.,bound-.37,bound}){AudioEngine engine;engine.prepare(rate,lag,false);double err=0,energy=0;for(int n=0;n<int(3*bound+2000);++n){const float target=float(std::sin(2*pi*.4*(n-lag))),reference=float(std::sin(2*pi*.4*n));auto out=engine.process({target,target},{reference,reference});if(n>int(2*bound+200)){double d=out.a[0]-out.b[0];err+=d*d;energy+=out.b[0]*out.b[0];}}check(std::sqrt(err/energy)<.001,"interpolation guard preserves quality at both delay limits");}}
 AudioEngine transition;transition.prepare(48000,0,false);for(int n=0;n<3000;++n)transition.process({float(std::sin(2*pi*.1*n)),0},{0,0});
 transition.setCorrection(237,false);double crossfadeError=0;
 for(int n=3000;n<4000;++n){auto out=transition.process({float(std::sin(2*pi*.1*n)),0},{0,0});double blend=std::min(1.0,(n-2999)/960.0),expected=(1-blend)*std::sin(2*pi*.1*(n-transition.getLatency()))+blend*std::sin(2*pi*.1*(n-transition.getLatency()+237));crossfadeError=std::max(crossfadeError,std::abs(out.a[0]-expected));}
 check(crossfadeError<1e-6 && !transition.isSettling(),"timing change blends fixed delay taps and settles within 20 ms");
 check(zeroLagCorrelation(nullptr,nullptr,0)==0,"empty verification block safely rejected");
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
