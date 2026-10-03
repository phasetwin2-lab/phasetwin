#include "AudioEngine.h"
#include "ScopeData.h"
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <limits>
#include <fstream>
#include <iomanip>
using namespace phasetwin;
int checks=0;
void require(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
float signal(double n,double rate){
    double result=0;
    // Non-harmonic, band-limited signal; can be evaluated at fractional times.
    for(int k=0;k<37;++k){double frequency=180.0+177.3*k+3.17*k*k;result+=std::sin(2*3.14159265358979323846*frequency*n/rate+0.717*k*k);}
    return float(result/37);
}
int main(){try{
    Estimator estimator;
    std::array<float,frameSize> a{},b{},postA{},postB{};
    std::ofstream evidence("alignment_metrics.csv");
    evidence<<std::setprecision(12);
    evidence<<"sample_rate,true_lag_samples,estimated_lag_samples,inverted,correlation_before,correlation_after,residual_lag_samples,normalized_rms_error\n";
    std::ofstream waves("alignment_waveforms.csv");
    waves<<"time_ms,raw_a,raw_b,corrected_a,delayed_b\n";
    for(int rate:{44100,48000,96000,192000})for(int direction:{-1,1})for(bool inverted:{false,true}){
        double lag=direction*(std::floor(rate*0.007)+0.37);
        // First estimate from a separate raw input window.
        for(int n=0;n<frameSize;++n){a[n]=(inverted?-1.f:1.f)*signal(n-lag,rate);b[n]=signal(n,rate);}
        auto e=estimator.analyse(a.data(),b.data(),int(rate*0.02),0.65);
        require(e.valid,"known band-limited delay rejected");require(std::abs(e.lag-lag)<0.02,"fractional lag accuracy");require(e.inverted==inverted,"polarity detection");
        AudioEngine engine;engine.prepare(rate,0,false);engine.setCorrection(e.lag,e.inverted);
        int warmup=rate*2;

        for(int n=0;n<warmup+frameSize;++n){
            std::array<float,2> target{(inverted?-1.f:1.f)*signal(n-lag,rate),0},ref{signal(n,rate),0};
            target[1]=0.4f*target[0];ref[1]=0.4f*ref[0];
            auto output=engine.process(target,ref);
            if(rate==48000 && direction==1 && inverted && n>=warmup && n<warmup+960)waves<<1000.0*(n-warmup)/rate<<','<<target[0]<<','<<ref[0]<<','<<output.a[0]<<','<<output.b[0]<<'\n';
            if(n>=warmup){postA[n-warmup]=output.a[0];postB[n-warmup]=output.b[0];require(std::abs(output.a[1]-0.4f*output.a[0])<1e-6,"stereo linked delay");}
        }
        auto residual=estimator.analyse(postA.data(),postB.data(),int(rate*0.02),0.65);
        double before=zeroLagCorrelation(a.data(),b.data(),frameSize),after=zeroLagCorrelation(postA.data(),postB.data(),frameSize);
        double error=0,energy=0;for(int n=0;n<frameSize;++n){double d=postA[n]-postB[n];error+=d*d;energy+=postB[n]*postB[n];}
        double rms=std::sqrt(error/energy);
        require(!engine.isSettling(),"correction failed to settle");require(residual.valid&&!residual.inverted&&std::abs(residual.lag)<0.2,"post-correction residual");require(after>0.999,"post-correction correlation");require(rms<0.05,"post-correction null error");
        evidence<<rate<<','<<lag<<','<<e.lag<<','<<inverted<<','<<before<<','<<after<<','<<residual.lag<<','<<rms<<'\n';
        std::cout<<"PASS rate="<<rate<<" lag="<<lag<<" invert="<<inverted<<" output correlation="<<after<<" residual="<<residual.lag<<" RMS error="<<rms<<'\n';
    }
    // An actually wrong manual correction must fail verification.
    AudioEngine wrong;wrong.prepare(48000,0,false);
    for(int n=0;n<frameSize+2000;++n){auto out=wrong.process({signal(n-240,48000),0},{signal(n,48000),0});if(n>=2000){postA[n-2000]=out.a[0];postB[n-2000]=out.b[0];}}
    auto residual=estimator.analyse(postA.data(),postB.data(),960,0.65);require(residual.valid&&std::abs(residual.lag-240)<0.15,"bad manual alignment falsely verified");
    wrong.setCorrection(240,false);require(wrong.isSettling(),"new delay falsely reported settled");
    for(int n=0;n<frameSize;++n)a[n]=b[n]=signal(n,48000);
    a[10]=std::numeric_limits<float>::quiet_NaN();require(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"NaN frame accepted");
    a[10]=std::numeric_limits<float>::infinity();require(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"infinite frame accepted");
    // Actual producer/consumer race test, including queue wraparound and full queue.
    ScopeQueue queue;ScopePacket packet;for(int i=0;i<63;++i){packet.generation=unsigned(i);require(queue.push(packet),"queue capacity");}
    require(!queue.push(packet),"full queue not bounded");for(int i=0;i<63;++i){require(queue.pop(packet)&&packet.generation==unsigned(i),"queue order");}require(!queue.pop(packet),"empty queue not bounded");
    constexpr int packets=3000;std::atomic<bool> bad{false};
    std::thread producer([&]{ScopePacket x;for(int i=0;i<packets;++i){x.generation=unsigned(i);for(int t=0;t<8;++t)for(int n=0;n<scopePacketSamples;++n)x.traces[t][n]=float(i+t+n);while(!queue.push(x))std::this_thread::yield();}});
    std::thread consumer([&]{ScopePacket x;for(int i=0;i<packets;++i){while(!queue.pop(x))std::this_thread::yield();if(x.generation!=unsigned(i))bad=true;for(int t=0;t<8;++t)for(int n=0;n<scopePacketSamples;++n)if(x.traces[t][n]!=float(i+t+n))bad=true;}});
    producer.join();consumer.join();require(!bad.load(),"scope queue torn or reordered samples");
    std::cout<<"PASS wrong correction, settling, invalid samples, scope capacity and concurrent transfer\n"<<checks<<" assertions passed\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
