#include "Alignment.h"
#include "ScopeHistory.h"
#include <iostream>
#include <stdexcept>
#include <random>
using namespace phasetwin;
int checks=0;
void check(bool pass,const char* msg){++checks;if(!pass)throw std::runtime_error(msg);std::cout<<"PASS "<<msg<<'\n';}
int main(){try{
    check(std::abs(scopeDurationMs(true,20,1,120,true)-500)<1e-9,"120 BPM one beat = 500 ms");
    check(std::abs(scopeDurationMs(true,20,0.25,90,true)-166.6666667)<1e-5,"90 BPM sixteenth note window");
    check(scopeDurationMs(true,20,1,0,false)==500,"missing tempo uses 120 BPM");
    check(scopeDurationMs(false,1700,1,120,true)==1700,"manual millisecond override");
    check(scopeDurationMs(true,20,4,30,true)==4000,"long tempo window capped at 4 seconds");
    check(scopeDurationMs(true,20,1,std::nan(""),true)==500,"invalid tempo fallback");
    std::array<float,frameSize> a{},r{},negA{},negB{};std::mt19937 rng(13);std::normal_distribution<float> random(0,0.2f);
    for(int n=0;n<frameSize;++n){r[n]=random(rng);a[n]=n>=53?r[n-53]:random(rng);negA[n]=-a[n];negB[n]=-r[n];}
    Estimator e;int channel=strongestPair(a.data(),negA.data(),r.data(),negB.data(),frameSize);
    auto estimate=e.analyse(channel?negA.data():a.data(),channel?negB.data():r.data(),960,0.65);
    check(estimate.valid&&std::abs(estimate.lag-53)<0.05,"anti-phase stereo no longer cancels analysis");
    std::array<float,frameSize> silence{};
    check(e.analyse(a.data(),silence.data(),960,0.65).reason==AnalysisReason::referenceSilent,"silent reference diagnosed");
    check(e.analyse(silence.data(),r.data(),960,0.65).reason==AnalysisReason::targetSilent,"silent main source diagnosed");
    check(strongestPair(silence.data(),a.data(),silence.data(),r.data(),frameSize)==1,"right-only matched pair selected");
    for(int n=0;n<frameSize;++n)a[n]=random(rng);
    check(e.analyse(a.data(),r.data(),960,0.65).reason==AnalysisReason::weakCorrelation,"unrelated sources diagnosed");
    for(int n=0;n<frameSize;++n)a[n]=r[n]=float(std::sin(2*3.141592653589793*n/64));
    check(e.analyse(a.data(),r.data(),960,0.65).reason==AnalysisReason::ambiguous,"periodic ambiguity diagnosed");
    ScopeHistory history;ScopePacket packet;packet.sampleRate=192000;
    for(int i=0;i<3100;++i){
        packet.firstSample=std::uint64_t(i)*scopePacketSamples;
        for(auto& trace:packet.traces)trace.fill(0);
        if(i==500){packet.traces[0][19]=0.95f;packet.traces[1][19]=-0.95f;}
        history.ingest(packet);
    }
    int count=history.availableSamples(4000);
    check(count==768000,"full 4000 ms history available at 192 kHz");
    auto peaks=history.range(0,0,0,count,count);check(std::abs(peaks.second-0.95f)<1e-6,"long view retains single-sample transient");
    auto mono=history.range(0,2,0,count,count);check(mono.first==0&&mono.second==0,"mono envelope preserves real cancellation");
    packet.firstSample+=1024;history.ingest(packet);
    check(history.availableSamples(4000)==256,"dropped packets reset history rather than compressing time");
    packet.firstSample+=256;packet.sampleRate=48000;history.ingest(packet);
    check(history.availableSamples(4000)==256,"sample-rate change resets time history");
    std::cout<<checks<<" workflow checks passed\n";
}catch(const std::exception& x){std::cerr<<"FAIL "<<x.what()<<'\n';return 1;}}
