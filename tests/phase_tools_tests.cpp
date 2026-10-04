#include "PhaseTools.h"
#include <iostream>
#include <stdexcept>
using namespace phasetwin;
void check(bool value,const char* name){if(!value)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
int main(){try{
 for(int rate:{44100,48000,96000,192000})for(double frequency:{40.0,80.0,400.0,3000.0}){
    PhaseRotator rotation;rotation.prepare(rate);rotation.configure(true,80,.707f);double input=0,output=0;
    for(int n=0;n<rate*2;++n){float x=float(.5*std::sin(2*3.141592653589793*frequency*n/rate));auto y=rotation.process({x,x});if(!std::isfinite(y[0]) || y[0]!=y[1])throw std::runtime_error("rotation stereo/finite");if(n>=rate){input+=double(x)*x;output+=double(y[0])*y[0];}}
    check(std::abs(10*std::log10(output/input))<.01,"steady-state all-pass magnitude across rates/frequencies");rotation.configure(false,80,.707f);float error=0;for(int n=0;n<rate;++n){float x=.3f;error=std::abs(rotation.process({x,x})[0]-x);}check(error<1e-6,"disabled rotation settles to neutral");
 }
 PhaseRotator centre;centre.prepare(48000);centre.configure(true,80,.707f);double dot=0,energy=0;for(int n=0;n<96000;++n){const float x=float(.5*std::sin(2*3.141592653589793*80*n/48000));auto y=centre.process({x,x});if(n>=48000){dot+=double(x)*y[0];energy+=double(x)*x;}}check(dot/energy<-.999,"all-pass centre gives 180 degree relationship");
 for(float q:{.2f,2.f}){PhaseRotator p;p.prepare(48000);p.configure(true,20,q);for(int n=0;n<48000;++n)p.process({0,0});double sum=0;for(int n=0;n<48000;++n){float y=p.process({n==0?1.f:0.f,0})[0];sum+=double(y)*y;}check(std::abs(sum-1)<1e-5,"all-pass impulse energy at extreme Q");}
 check(phaseCurveConnects(20,25,1,1),"adjacent supported phase measurements connect");check(!phaseCurveConnects(179,-179,1,1),"wrapped phase breaks instead of crossing zero");check(!phaseCurveConnects(20,25,0,1),"unsupported gap never gets bridged");check(!phaseCurveConnects(20,25,1,.1f),"low-support phase does not become a curve");
 SpectralAnalyzer analysis;std::array<float,frameSize> a{},b{},post{};
 for(int n=0;n<frameSize;++n){b[n]=float(.5*std::sin(2*3.141592653589793*187.5*n/48000));a[n]=-b[n];post[n]=b[n];}
 auto spectrum=analysis.analyse(a.data(),b.data(),post.data(),b.data(),48000);int found=0;for(int band=0;band<spectralBands;++band)if(spectrum.confidence[band]>.9){check(std::abs(std::abs(spectrum.before[band])-180)<.01 && std::abs(spectrum.after[band])<.01,"spectral before inversion and corrected phase");++found;}check(found>0,"supported spectral bands detected");a.fill(0);b.fill(0);post.fill(0);spectrum=analysis.analyse(a.data(),b.data(),post.data(),b.data(),48000);for(float support:spectrum.confidence)check(support==0,"silence gives no spectral evidence");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
