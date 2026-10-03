#include "Learning.h"
#include "AudioEngine.h"
#include <iostream>
#include <stdexcept>
#include <random>
using namespace phasetwin;
int checks=0;
void check(bool pass,const char* name){++checks;if(!pass)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
Estimate evidence(double lag,bool polarity=false){Estimate e;e.lag=lag;e.confidence=0.95;e.inverted=polarity;e.valid=true;e.reason=AnalysisReason::ok;e.peakMargin=0.3;return e;}
int main(){try{
    Learning learn;check(!learn.result(48000,false).reliable,"no evidence cannot auto apply");
    learn.add(evidence(73));check(!learn.result(48000,false).reliable,"one good peak is insufficient");
    for(double x:{73.01,72.99,73.02,73.0}){learn.add(evidence(x));}
    auto r=learn.result(48000,false);
    check(r.reliable&&std::abs(r.lag-73)<0.02&&r.confidence>0.9,"consistent multi-window sub-sample candidate");
    learn.reset();for(int i=0;i<8;++i)learn.add(evidence(i*96));check(!learn.result(48000,false).reliable,"moving timing rejected despite strong individual peaks");
    learn.reset();for(int i=0;i<8;++i)learn.add(evidence(10,i%2!=0));check(!learn.result(48000,false).reliable,"contradictory polarities rejected");
    learn.reset();for(int i=0;i<6;++i)learn.add(evidence(-15.37,true));r=learn.result(48000,false);check(r.reliable&&r.inverted&&r.lag==-15.37,"negative fractional lag and polarity consensus");
    auto bad=Estimate{};bad.reason=AnalysisReason::weakCorrelation;for(int i=0;i<70;++i)learn.add(bad);check(!learn.result(48000,false).reliable,"rolling history cannot retain stale reliable evidence forever");
    check(coherenceScore(1)==100&&coherenceScore(0)==0&&coherenceScore(-1)==0,"score definition is separate from learn confidence");
    std::array<float,frameSize> input{},filtered{},a{},b{},fa{},fb{};
    for(int i=0;i<frameSize;++i)input[i]=float(std::sin(2*3.141592653589793*i*5000/48000));
    AnalysisBand::filter(input.data(),filtered.data(),frameSize,48000,180);double ei=0,ef=0;for(int i=1000;i<frameSize;++i){ei+=input[i]*input[i];ef+=filtered[i]*filtered[i];}
    check(ef/ei<0.00001,"kick/bass analysis excludes high-frequency content");
    AnalysisBand::StereoBand stereoA{},stereoB{};
    for(int i=0;i<frameSize;++i){a[i]=10*input[i];b[i]=float(std::sin(2*3.141592653589793*i*70/48000));}
    check(AnalysisBand::selectFilteredChannel(a.data(),b.data(),a.data(),b.data(),stereoA,stereoB,48000,180)==1,"low-end selection ignores louder high-frequency channel");
    input.fill(1);AnalysisBand::filter(input.data(),filtered.data(),frameSize,48000,180);check(std::abs(filtered.back())<0.00001,"analysis band removes DC");
    // Musical kick-like transient and a delayed bass-domain copy, measured over several hits.
    std::mt19937 rng(58);std::normal_distribution<float> noise(0,0.02f);Estimator estimator;learn.reset();
    for(int frame=0;frame<6;++frame){
        for(int i=0;i<frameSize;++i){double t=double(i-500)/48000; b[i]=t>=0?float(std::exp(-t*30)*(std::sin(2*3.141592653589793*(70*t+90*t*t))+0.2*std::sin(2*3.141592653589793*137*t))):0; b[i]+=noise(rng);}
        for(int i=0;i<frameSize;++i)a[i]=i>=111?-0.8f*b[i-111]:0;
        AnalysisBand::filter(a.data(),fa.data(),frameSize,48000,180);AnalysisBand::filter(b.data(),fb.data(),frameSize,48000,180);
        learn.add(estimator.analyse(fa.data(),fb.data(),960,0.4,int(48000.0/(3*180))));
    }
    r=learn.result(48000,true);check(r.reliable&&r.inverted&&std::abs(r.lag-111)<0.5,"low-end transient learning preserves known timing and polarity");
    for(int i=0;i<frameSize;++i)fa[i]=fb[i]=float(std::sin(2*3.141592653589793*i*80/48000));
    check(!estimator.analyse(fa.data(),fb.data(),960,0.4,int(48000.0/(3*180))).valid,"low-band periodic ambiguity still rejected");
    AudioEngine engine;engine.prepare(48000,7,false);double correctedError=0,neutralError=0;
    for(int i=0;i<2000;++i){auto out=engine.process({i==107?1.f:0.f,0},{i==100?1.f:0.f,0});correctedError+=std::abs(out.a[0]-out.b[0]);neutralError+=std::abs(out.neutralA[0]-out.b[0]);}
    check(correctedError<1e-6&&neutralError>1.9,"neutral A/B keeps baseline latency and exposes real correction");
    std::cout<<checks<<" learning checks passed\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
