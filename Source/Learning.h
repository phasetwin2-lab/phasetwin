#pragma once
#include "Alignment.h"
#include <array>
namespace phasetwin {
struct LearnedCorrection { double lag=0,confidence=0; bool inverted=false,reliable=false; int accepted=0; };
// Confidence is a heuristic combining evidence, peak uniqueness and temporal stability.
// It is not a calibrated probability, and does not depend on output score.
class Learning {
    std::array<Estimate,64> candidates{};
    int used=0,next=0;
public:
    void reset(){used=next=0;}
    void add(const Estimate& e){
        if(e.reason==AnalysisReason::targetSilent || e.reason==AnalysisReason::referenceSilent)return;
        candidates[next]=e;next=(next+1)%int(candidates.size());used=std::min(used+1,int(candidates.size()));
    }
    LearnedCorrection result(double sr,bool lowEnd)const{
        LearnedCorrection r;int accepted=0,negative=0;
        for(int i=0;i<used;++i)if(candidates[i].valid){++accepted;negative+=candidates[i].inverted?1:0;}
        r.accepted=accepted;if(accepted==0)return r;r.inverted=negative>accepted/2;
        std::array<double,64> lags{},deviations{};int count=0;double strength=0,uniqueness=0;
        for(int i=0;i<used;++i)if(candidates[i].valid && candidates[i].inverted==r.inverted){lags[count++]=candidates[i].lag;strength+=candidates[i].confidence;uniqueness+=std::clamp(0.5+0.5*candidates[i].peakMargin/0.1,0.0,1.0);}
        std::sort(lags.begin(),lags.begin()+count);r.lag=lags[count/2];
        for(int i=0;i<count;++i)deviations[i]=std::abs(lags[i]-r.lag);
        std::sort(deviations.begin(),deviations.begin()+count);
        const double spreadMs=1000.0*deviations[count/2]/sr;
        const double consistency=std::exp(-spreadMs/(lowEnd?0.75:0.15));
        const double agreement=double(count)/accepted,acceptance=double(accepted)/used;
        r.confidence=std::clamp(std::sqrt(strength/count)*std::sqrt(uniqueness/count)*consistency*agreement*acceptance*std::min(1.0,count/4.0),0.0,1.0);
        r.reliable=count>=3 && agreement>=0.75 && r.confidence>=(lowEnd?0.45:0.55);
        return r;
    }
};
// These filters affect analysis only. Both sources receive the same response.
class AnalysisBand {
    struct Biquad {
        double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        void configure(double sr,double frequency,bool highpass){
            const double w=2*3.14159265358979323846*frequency/sr,c=std::cos(w),alpha=std::sin(w)/std::sqrt(2.0),a0=1+alpha;
            b0=(highpass?(1+c):(1-c))/2/a0;b1=(highpass?-(1+c):(1-c))/a0;b2=b0;a1=-2*c/a0;a2=(1-alpha)/a0;z1=z2=0;
        }
        float process(float x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return float(y);}
    };
public:
    using StereoBand=std::array<std::array<float,frameSize>,2>;
    static int selectFilteredChannel(const float* al,const float* ar,const float* bl,const float* br,StereoBand& a,StereoBand& b,double sr,double cutoff){
        filter(al,a[0].data(),frameSize,sr,cutoff);filter(ar,a[1].data(),frameSize,sr,cutoff);
        filter(bl,b[0].data(),frameSize,sr,cutoff);filter(br,b[1].data(),frameSize,sr,cutoff);
        return strongestPair(a[0].data(),a[1].data(),b[0].data(),b[1].data(),frameSize);
    }
    static void filter(const float* input,float* output,int count,double sr,double cutoff){
        Biquad hp,lp;hp.configure(sr,25,true);lp.configure(sr,std::clamp(cutoff,50.0,500.0),false);
        for(int i=0;i<count;++i)output[i]=lp.process(hp.process(input[i]));
    }
};
inline float coherenceScore(double correlation){return float(100*std::max(0.0,correlation));}
}
