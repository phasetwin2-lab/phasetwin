#include "Alignment.h"
#include <iostream>
#include <random>
#include <stdexcept>
#include <array>
void check(bool pass,const char* name){if(!pass)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
int main(){
 try{
    using namespace phasetwin;std::array<float,frameSize> a{},b{};std::mt19937 rng(42);std::normal_distribution<float> noise(0,0.2f);
    for(auto& x:b){x=noise(rng);}
    Estimator estimator;
    for(int d:{-950,-137,0,73,950}){for(int i=0;i<frameSize;++i){int j=i-d;a[i]=(j>=0&&j<frameSize)?b[j]:noise(rng);}auto e=estimator.analyse(a.data(),b.data(),960,0.65);check(e.valid&&std::abs(e.lag-d)<0.05&&!e.inverted,"signed broadband delay");}
    for(int i=0;i<frameSize;++i){a[i]=i>=123?-0.7f*b[i-123]:noise(rng);}
    auto e=estimator.analyse(a.data(),b.data(),960,0.65);check(e.valid&&e.inverted&&std::abs(e.lag-123)<0.05,"inverted polarity with level mismatch");
    for(int i=0;i<frameSize;++i){a[i]=i>=41?0.7f*b[i-40]+0.3f*b[i-41]:noise(rng);}
    e=estimator.analyse(a.data(),b.data(),960,0.65);check(e.valid&&e.lag>40&&e.lag<40.5,"fractional peak interpolation");
    for(auto& x:a){x=noise(rng);}
    check(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"unrelated sources rejected");
    a.fill(0);check(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"silence rejected");
    for(int i=0;i<frameSize;++i){a[i]=b[i]=float(std::sin(2*3.141592653589793*i/64));}
    check(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"ambiguous periodic source rejected");
    for(auto& x:b){x=noise(rng);}
    for(int i=0;i<frameSize;++i){a[i]=i>=960?b[i-960]:noise(rng);}
    check(!estimator.analyse(a.data(),b.data(),960,0.65).valid,"search boundary rejected");
    for(int rate:{44100,48000,96000,192000}){
        int bound=int(rate*0.020),lag=int(rate*0.015);
        for(int i=0;i<frameSize;++i){a[i]=0.1f+(i>=lag?0.5f*b[i-lag]:noise(rng))+0.05f*noise(rng);}
        e=estimator.analyse(a.data(),b.data(),bound,0.65);
        check(e.valid&&std::abs(e.lag-lag)<0.1,"sample rate / DC / noisy input");
    }
    Delay delay;delay.prepare(64);delay.push(1);check(std::abs(delay.read(0)-1)<1e-6,"zero delay");delay.push(0);check(std::abs(delay.read(1)-1)<1e-6,"one sample delay");check(std::abs(delay.read(0.5)-0.5)<1e-6,"fractional delay read");
    Delay cubic;cubic.prepare(16);for(int i=0;i<32;++i)cubic.push(float(i*i));
    check(std::abs(cubic.read(1.5)-29.5*29.5)<1e-5,"cubic interpolation and ring wraparound");
    std::array<Delay,2> paths;for(auto& p:paths)p.prepare(128);double error=0;for(int n=0;n<500;++n){float ref=n==100?1.f:0.f,target=n==107?1.f:0.f;paths[0].push(target);paths[1].push(ref);error+=std::abs(paths[0].read(32-7)-paths[1].read(32));}check(error<1e-6,"causal correction aligns both paths");
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
