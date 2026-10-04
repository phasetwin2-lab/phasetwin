#include "Ducking.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace phasetwin;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);std::cout<<"PASS "<<text<<'\n';}
int main(){try{
 Ducker off;off.prepare(48000);float offGain=0;for(int i=0;i<2000;++i)offGain=off.process({1,-1});check(offGain==1,"disabled default passes unity gain");
 Ducker silence;silence.prepare(48000);silence.configure(true,100,100);float gain=0;for(int i=0;i<4800;++i)gain=silence.process({0,0});check(gain==1,"silent sidechain does not duck");
 for(int rate:{44100,48000,96000,192000})for(float amount:{0.f,50.f,100.f}){Ducker d;d.prepare(rate);d.configure(true,amount,50);for(int i=0;i<rate;++i){gain=d.process({.8f,-.8f});if(!std::isfinite(gain) || gain<.06309f || gain>1)throw std::runtime_error("gain bounds");}check(std::abs(d.reductionDb()-.24f*amount)<.01f,"selected depth at sustained full activity across sample rates");}
 Ducker left,right,opposite;for(auto* d:{&left,&right,&opposite}){d->prepare(48000);d->configure(true,75,50);}for(int i=0;i<10000;++i){float a=left.process({.5f,0}),b=right.process({0,.5f}),c=opposite.process({.5f,-.5f});if(a!=b || b!=c)throw std::runtime_error("stereo detector cancellation or imbalance");}check(true,"linked detector treats left/right and opposite polarity equally");
 Ducker soft,hard;soft.prepare(48000);hard.prepare(48000);soft.configure(true,100,0);hard.configure(true,100,100);for(int i=0;i<96;++i){soft.process({.5f,.5f});hard.process({.5f,.5f});}check(hard.reductionDb()>soft.reductionDb()+1,"harshness gives faster duck onset");
 for(int i=0;i<48000;++i){soft.process({.5f,.5f});hard.process({.5f,.5f});}for(int i=0;i<4800;++i){soft.process({0,0});hard.process({0,0});}check(hard.reductionDb()<soft.reductionDb(),"harshness gives shorter release");
 left.configure(false,100,100);for(int i=0;i<9600;++i)gain=left.process({1,1});check(std::abs(gain-1)<1e-6,"disabling smoothly restores unity with active reference");
 Ducker fast,slow;fast.prepare(48000);slow.prepare(48000);fast.configure(true,100,50,true,.1f,20);slow.configure(true,100,50,true,100,1000);for(int i=0;i<480;++i){fast.process({.5f,.5f});slow.process({.5f,.5f});}check(fast.reductionDb()>slow.reductionDb()+2,"explicit attack independently controls onset");
 for(int i=0;i<96000;++i){fast.process({.5f,.5f});slow.process({.5f,.5f});}for(int i=0;i<4800;++i){fast.process({0,0});slow.process({0,0});}check(fast.reductionDb()<slow.reductionDb(),"explicit release independently controls recovery");
 Ducker sensitive,insensitive;for(auto* d:{&sensitive,&insensitive})d->prepare(48000);sensitive.configure(true,100,50,true,3,125,12);insensitive.configure(true,100,50,true,3,125,-12);for(int i=0;i<48000;++i){sensitive.process({.04f,.04f});insensitive.process({.04f,.04f});}check(sensitive.reductionDb()>insensitive.reductionDb()+5,"sensitivity changes quiet-sidechain response without output boost");
 Ducker simpleA,simpleB;simpleA.prepare(48000);simpleB.prepare(48000);simpleA.configure(true,100,50,false,.1f,10);simpleB.configure(true,100,50,false,100,1000);for(int i=0;i<4800;++i)if(simpleA.process({.1f,.1f})!=simpleB.process({.1f,.1f}))throw std::runtime_error("simple mode used advanced timing");check(true,"simple mode ignores advanced timing values");
 Ducker invalid;invalid.prepare(48000);invalid.configure(true,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity());gain=invalid.process({std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()});check(std::isfinite(gain) && gain==1,"nonfinite controls and sidechain remain safe");
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
