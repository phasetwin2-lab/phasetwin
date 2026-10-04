#include "SessionAnalysis.h"
#include <iostream>
#include <stdexcept>
#include <random>
using namespace phasetwin;
static int checks=0;
static void require(bool ok,const char* name){++checks;if(!ok)throw std::runtime_error(name);}
static void fillKick(KickData& d,int delay,bool flip,double hz=60){d={};d.count=kickCaptureSize;for(int h=1;h<8;++h)for(int n=0;n<1500;++n){int i=h*3000+n;if(i>=d.count)break;const float x=float(std::sin(2*3.141592653589793*hz*n/kickRate)*std::exp(-double(n)/300));d.audio[2][i]=d.audio[3][i]=x;if(i+delay>=0 && i+delay<d.count)d.audio[0][i+delay]=d.audio[1][i+delay]=flip?-x:x;}}
int main(){try{
    VerificationAccumulator acc;for(int i=0;i<5;++i)acc.add(-.3,.7);require(acc.result().state==3,"improved classification");acc.reset();for(int i=0;i<5;++i)acc.add(.8,.1);require(acc.result().state==4,"worsened classification");acc.reset();for(int i=0;i<5;++i)acc.add(.5,.505);require(acc.result().state==5,"unchanged classification");acc.reset();acc.add(.1,.9);require(acc.result().state==6,"insufficient evidence");acc.reset();for(int i=0;i<4;++i)acc.add(.1,i%2?.9:-.6);require(acc.result().state==6,"conflicting evidence");acc.reset();acc.add(NAN,.5);require(acc.result().count==0,"nonfinite verification");
    auto before=std::make_unique<KickData>(),after=std::make_unique<KickData>();fillKick(*before,24,true);fillKick(*after,0,false);
    auto m=measureKick(*after);require(m.valid && m.mean>.99,"fixed kick measurement");auto v=verifyKickPair(*before,*after);require(v.state==3 && v.count>=3,"fresh kick improvement");require(verifyKickPair(*after,*before).state==4,"fresh kick worsening");require(verifyKickPair(*after,*after).state==5,"fresh kick unchanged");before->audio[0][10]=NAN;require(!measureKick(*before).valid,"nonfinite kick rejected");*before={};before->count=kickCaptureSize;require(verifyKickPair(*before,*after).state==6,"silent kick unmeasurable");
    auto bank=std::make_unique<SectionBank>();KickAnalyzer analyzer;bank->begin(true,kickRate);KickConfig config;
    for(int s=0;s<3;++s){fillKick(*before,24,true,55+s*5);auto r=analyzer.analyse(*before,config);require(r.reliable,"section candidate unreliable");bank->addKick(*before,r);}
    auto joint=bank->analyse(config);require(joint.reliable && joint.inverted && std::abs(joint.lag-24)<1,"common kick correction");for(int i=0;i<joint.count;++i)require(joint.gains[i]>.02,"candidate not checked across sections");
    bank->clear();bank->begin(true,kickRate);fillKick(*before,0,true);bank->addKick(*before,analyzer.analyse(*before,config));require(!bank->analyse(config).reliable,"one section accepted");fillKick(*before,0,false);bank->addKick(*before,analyzer.analyse(*before,config));require(!bank->analyse(config).reliable,"contradictory polarity sections accepted");
    bank->clear();bank->begin(true,kickRate);fillKick(*before,24,true);for(int s=0;s<4;++s)require(bank->addKick(*before,analyzer.analyse(*before,config)),"section capacity lost");require(!bank->addKick(*before,{}),"unbounded bank");config.preserveGroove=true;config.grooveLimit=12;require(!bank->analyse(config).reliable,"common candidate ignored groove cap");
    bank->clear();bank->begin(false,48000);SessionStereo a{},b{};std::mt19937 rng(31);std::uniform_real_distribution<float> noise(-.5,.5);for(int s=0;s<2;++s){for(int n=0;n<frameSize;++n){b[0][n]=b[1][n]=noise(rng);a[0][n]=a[1][n]=n>=12?-b[0][n-12]:0;}Estimate e;e.valid=true;e.lag=12;e.inverted=true;e.confidence=.95;e.peakMargin=.2;for(int f=0;f<5;++f)bank->addSourceFrame(a,b,e);bank->finishSource();}config={};config.search=960;auto source=bank->analyse(config);require(source.reliable && source.inverted && std::abs(source.lag-12)<1e-9,"common same-source correction");
    // Fresh measurement uses the real matched-latency output, not predicted input shifts.
    for(double sr:{44100.0,48000.0,96000.0}){
        const int delay=int(std::round(sr*.004));AudioEngine engine;engine.prepare(sr,delay,true);KickCapture bc,ac;bc.prepare(sr,180);ac.prepare(sr,180);*before={};*after={};
        auto wave=[&](int t){if(t<0)return 0.0f;int p=t%int(sr*.5);return p<int(sr*.2)?float(std::sin(2*3.141592653589793*60*p/sr)*std::exp(-double(p)/(sr*.05))):0.0f;};
        for(int n=0;n<int(sr*4);++n){const float a=-wave(n-delay),b=wave(n);auto out=engine.process({a,a},{b,b});bc.push(*before,out.neutralA,out.b);ac.push(*after,out.a,out.b);}
        auto fresh=verifyKickPair(*before,*after);require(fresh.state==3 && fresh.after>fresh.before+.02,"real output fresh verification failed across rates");
    }
    bank->clear();require(bank->count()==0,"clear retains sections");require(!bank->analyse(config).reliable,"empty collection accepted");
    std::cout<<checks<<" session analysis checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
