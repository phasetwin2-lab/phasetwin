#include "Learning.h"
#include <random>
#include <iostream>
#include <stdexcept>
using namespace phasetwin;
void check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
int main(){try{
 Estimator estimator;Learning session;std::array<float,frameSize> a{},b{};
 std::mt19937 rng(1729);std::normal_distribution<float> noise(0,.2f);
 for(int rate:{44100,48000,96000,192000})for(int direction:{-1,1})for(bool polarity:{false,true}){
  session.reset();int lag=direction*int(rate*.003);
  for(int frame=0;frame<5;++frame){for(auto& x:b)x=noise(rng);
   for(int n=0;n<frameSize;++n){auto sample=[&](int i){return i>=0 && i<frameSize?b[i]:noise(rng);};a[n]=.12f+(polarity?-1.f:1.f)*(.7f*sample(n-lag)+.14f*sample(n-lag-137))+.03f*noise(rng);}
   session.add(estimator.analyse(a.data(),b.data(),int(rate*.02),.65));
  }
  auto result=session.result(rate,false);check(result.reliable && result.inverted==polarity && std::abs(result.lag-lag)<.15,"multi-window timing/polarity with gain, DC, noise and weaker reflection");
 }
 for(auto& x:b)x=noise(rng);
 for(int n=0;n<frameSize;++n)a[n]=n>=300?.5f*(b[n-100]+b[n-300]):0;
 check(!estimator.analyse(a.data(),b.data(),960,.5).valid,"equally strong reflection peaks remain ambiguous");
 for(auto& x:a){x=noise(rng);}
 check(!estimator.analyse(a.data(),b.data(),960,.65).valid,"unrelated microphones rejected");
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
