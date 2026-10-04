#pragma once
#include "Learning.h"
#include <vector>
namespace phasetwin {
constexpr int kickRate=6000,kickCaptureSize=kickRate*4;
struct KickData { std::array<std::array<float,kickCaptureSize>,4> audio{};int count=0; };
// Capture is audio-thread owned until publication; processing runs on the worker.
class KickCapture {
    struct LowPass {
        double b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        void prepare(double sr,double cutoff,double q,bool hp=false){double w=2*3.141592653589793*cutoff/sr,c=std::cos(w),alpha=std::sin(w)/(2*q),a0=1+alpha;b0=(hp?(1+c):(1-c))/2/a0;b1=(hp?-(1+c):(1-c))/a0;b2=b0;a1=-2*c/a0;a2=(1-alpha)/a0;z1=z2=0;}
        float process(float x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return float(y);}
    };
    std::array<std::array<LowPass,3>,4> filters;
    double phase=0,rate=48000;
public:
    void prepare(double sr,double cutoff){rate=sr;phase=0;for(auto& f:filters){f[0].prepare(sr,cutoff,0.5411961);f[1].prepare(sr,cutoff,1.30656296);f[2].prepare(sr,25,0.70710678,true);}}
    bool push(KickData& data,const std::array<float,2>& a,const std::array<float,2>& b){
        std::array<float,4> y{};for(int ch=0;ch<4;++ch)y[ch]=filters[ch][1].process(filters[ch][0].process(filters[ch][2].process(ch<2?a[ch]:b[ch-2])));
        phase+=kickRate;if(phase<rate)return false;phase-=rate;
        if(data.count>=kickCaptureSize)return true;
        for(int ch=0;ch<4;++ch){data.audio[ch][data.count]=y[ch];}
        ++data.count;
        return data.count==kickCaptureSize;
    }
};
struct KickConfig { int search=120;double currentLag=0,trim=0;bool currentInverted=false,manualInverted=false,allowTiming=true,allowPolarity=true;bool preserveGroove=false;double grooveLimit=12; };
struct KickResult { double lag=0,confidence=0,before=0,after=0,neutral=0,improvement=0;bool inverted=false,reliable=false;int hits=0,channel=0,reason=0; }; // reason: 0 ambiguous, 1 insufficient hits, 2 little overlap, 3 no benefit, 4 groove guard
inline bool createsGrooveGap(const float* a,const float* b,int count,int begin,int end,double baseline,double candidate){
    auto read=[&](double p){const int i=int(std::floor(p));return i>=0 && i+1<count?double(a[i])+(p-i)*(a[i+1]-a[i]):0.0;};
    double peak=0;for(int n=begin;n<end;++n){const double x=read(n+baseline);peak=std::max(peak,x*x+double(b[n])*b[n]);}
    if(peak<1e-9)return false;
    constexpr int bin=12;const double threshold=peak*.005*bin;
    for(int n=begin;n+bin<=end;n+=bin){double before=0,after=0;for(int k=0;k<bin;++k){double y=b[n+k],x=read(n+k+baseline),z=read(n+k+candidate);before+=x*x+y*y;after+=z*z+y*y;}if(before>threshold*2 && after<threshold)return true;}
    return false;
}
class KickAnalyzer {
    static double read(const float* a,int count,double position){int i=int(std::floor(position));if(i<0 || i+1>=count)return 0;double f=position-i;return a[i]*(1-f)+a[i+1]*f;}
std::array<double,kickRate/8> weights{};
public:
    KickAnalyzer(){for(int n=0;n<kickRate/8;++n)weights[n]=std::exp(-double(n)/(kickRate*0.045));}
    KickResult analyse(const KickData& data,const KickConfig& config){
        KickResult result;if(data.count<0 || data.count>kickCaptureSize || config.search<1 || config.search>kickRate/50 || !std::isfinite(config.currentLag) || !std::isfinite(config.trim) || !std::isfinite(config.grooveLimit) || config.grooveLimit<0)return result;
        for(const auto& channel:data.audio)for(int i=0;i<data.count;++i)if(!std::isfinite(channel[i]))return result;
        result.channel=strongestPair(data.audio[0].data(),data.audio[1].data(),data.audio[2].data(),data.audio[3].data(),data.count);
        const auto* a=data.audio[result.channel].data();const auto* b=data.audio[2+result.channel].data();
        if(data.count<kickRate)return result;
        // Adaptive energy onset detector. Refractory time prevents tail retriggers.
        std::vector<int> hits;double fast=0,slow=0,peakEnergy=0;int refractory=0;
        for(int i=0;i<data.count;++i)peakEnergy=std::max(peakEnergy,double(b[i])*b[i]);
        if(peakEnergy<1e-9)return result;
        for(int i=0;i<data.count;++i){double energy=double(b[i])*b[i];fast+=0.04*(energy-fast);slow+=0.001*(energy-slow);
            if(refractory>0)--refractory;
            if(refractory==0 && fast>std::max(peakEnergy*0.008,slow*2.8)){
                if(i>=kickRate/50+12 && i+kickRate/50+kickRate/8<data.count)hits.push_back(i);
                refractory=kickRate/8;
            }
        }
        result.hits=int(hits.size());if(hits.size()<3){result.reason=1;return result;}
        auto event=[&](int onset,double lag,bool inverted){double ea=0,eb=0,ab=0;const double sign=inverted?-1:1;
            for(int n=0;n<kickRate/8;++n){double weight=weights[n],x=read(a,data.count,onset+n+lag)*sign,y=b[onset+n];ea+=weight*x*x;eb+=weight*y*y;ab+=weight*x*y;}
            return ea+eb>1e-10?std::clamp(2*ab/(ea+eb),-1.0,1.0):0.0;};
        auto objective=[&](double lag,bool inverted){double sum=0;for(int h:hits)sum+=event(h,lag,inverted);return sum/hits.size();};
        const bool baselineSign=config.currentInverted!=config.manualInverted;
        const double baselineLag=std::clamp(config.currentLag+config.trim,-double(kickRate/50),double(kickRate/50));
        bool grooveRejected=false;
        auto safeGroove=[&](double lag){if(!config.preserveGroove)return true;for(std::size_t h=1;h<hits.size();++h)if(createsGrooveGap(a,b,data.count,hits[h-1],hits[h],baselineLag,lag)){grooveRejected=true;return false;}return true;};
        result.before=objective(baselineLag,baselineSign);result.neutral=objective(0,false);
        auto training=[&](double lag,bool inverted){double sum=0;int count=0;for(std::size_t i=0;i<hits.size();i+=2){sum+=event(hits[i],lag,inverted);++count;}return sum/count;};
        double bestLag=baselineLag,bestValue=training(baselineLag,baselineSign);bool bestSign=baselineSign;
        for(int polarity=0;polarity<(config.allowPolarity?2:1);++polarity){bool sign=config.allowPolarity?polarity!=0:baselineSign;
            const int limit=config.preserveGroove?std::min(config.search,int(std::max(1.0,config.grooveLimit))):config.search;
            int lo=config.allowTiming?std::max(-config.search,int(std::ceil(baselineLag-limit))):int(std::floor(baselineLag)),hi=config.allowTiming?std::min(config.search,int(std::floor(baselineLag+limit))):lo;
            for(int lag=lo;lag<=hi;++lag){double actual=config.allowTiming?double(lag):baselineLag;if(!safeGroove(actual))continue;double value=training(actual,sign);
                // A small timing penalty favors keeping the groove when gains are equivalent.
                double merit=value-0.0001*std::abs(actual-baselineLag),bestMerit=bestValue-0.0001*std::abs(bestLag-baselineLag);
                if(merit>bestMerit+1e-8){bestValue=value;bestLag=actual;bestSign=sign;}
            }
        }
        const double coarseLag=bestLag;
        if(config.allowTiming)for(int step=-9;step<=9;++step){double lag=std::clamp(coarseLag+step*0.1,-double(config.search),double(config.search));if(config.preserveGroove && (std::abs(lag-baselineLag)>config.grooveLimit || !safeGroove(lag)))continue;double value=training(lag,bestSign);if(value>bestValue){bestValue=value;bestLag=lag;}}
        int improved=0,harmed=0;double meanGain=0;
        for(int h:hits){double gain=event(h,bestLag,bestSign)-event(h,baselineLag,baselineSign);meanGain+=gain;improved+=gain>0.01;harmed+=gain<-0.02;}
        meanGain/=hits.size();
        const double agreement=double(improved)/hits.size(),support=std::min(1.0,hits.size()/5.0);
        result.confidence=std::clamp(agreement*support,0.0,1.0);result.improvement=meanGain;
        result.lag=bestLag-config.trim;result.inverted=bestSign!=config.manualInverted;result.after=objective(bestLag,bestSign);
        result.reliable=meanGain>0.02 && agreement>=0.75 && harmed==0 && result.confidence>=0.55;
        // An already constructive baseline needs no change, and is not a failed search.
        if(!result.reliable && result.before>0.65 && std::abs(result.after-result.before)<0.02){result.lag=config.currentLag;result.inverted=config.currentInverted;result.after=result.before;result.reliable=true;result.confidence=support;}
        if(!result.reliable)result.reason=grooveRejected?4:std::max(std::abs(result.before),std::abs(result.after))<.03?2:meanGain<=.02?3:0;
        else if(meanGain<=.02)result.reason=3;
        return result;
    }
};
inline float kickScore(double interaction){return float(50*(1+std::clamp(interaction,-1.0,1.0)));}
}
