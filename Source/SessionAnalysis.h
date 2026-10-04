#pragma once
#include "KickAlignment.h"
#include "AudioEngine.h"
#include <memory>
#include <vector>
namespace phasetwin {
constexpr int maxSections=4;
// Fixed timing/polarity measurement, not another alignment search.
struct InteractionMeasurement {std::array<double,64> events{};std::array<int,64> onsets{};int hits=0;double mean=0;bool valid=false;};
inline InteractionMeasurement measureKick(const KickData& data,double lag=0,bool inverted=false){
    InteractionMeasurement r;if(data.count<kickRate || data.count>kickCaptureSize || !std::isfinite(lag) || std::abs(lag)>120)return r;
    for(const auto& c:data.audio)for(int i=0;i<data.count;++i)if(!std::isfinite(c[i]))return r;
    const int ch=strongestPair(data.audio[0].data(),data.audio[1].data(),data.audio[2].data(),data.audio[3].data(),data.count);
    const float* a=data.audio[ch].data();const float* b=data.audio[ch+2].data();double peak=0,fast=0,slow=0;int refractory=0;
    for(int i=0;i<data.count;++i)peak=std::max(peak,double(b[i])*b[i]);
    if(peak<1e-9)return r;
    for(int i=0;i<data.count && r.hits<64;++i){double e=double(b[i])*b[i];fast+=.04*(e-fast);slow+=.001*(e-slow);if(refractory>0)--refractory;
        if(refractory==0 && fast>std::max(peak*.008,slow*2.8)){
            refractory=kickRate/8;if(i<kickRate/50+12 || i+kickRate/50+kickRate/8>=data.count)continue;
            double ea=0,eb=0,ab=0;
            for(int n=0;n<kickRate/8;++n){double p=i+n+lag;int k=int(std::floor(p));double x=k>=0 && k+1<data.count?a[k]+(p-k)*(a[k+1]-a[k]):0;x*=inverted?-1:1;double y=b[i+n],w=std::exp(-double(n)/(kickRate*.045));ea+=w*x*x;eb+=w*y*y;ab+=w*x*y;}
            if(ea<1e-8 || eb<1e-8 || 2*std::sqrt(ea*eb)/(ea+eb)<.05)continue;
            r.onsets[r.hits]=i;r.events[r.hits++]=std::clamp(2*ab/(ea+eb),-1.0,1.0);
        }
    }
    for(int i=0;i<r.hits;++i)r.mean+=r.events[i];
    if(r.hits)r.mean/=r.hits;
    r.valid=r.hits>=3;return r;
}
// 0 off, 1 waiting for normal audition/settling, 2 measuring, 3 improved,
// 4 worsened, 5 no clear difference, 6 insufficient or conflicting evidence.
struct VerificationResult {int state=6,count=0;double before=0,after=0,confidence=0;};
class VerificationAccumulator {
    double before=0,after=0;int count=0,up=0,down=0;
public:
    void reset(){before=after=0;count=up=down=0;}
    void add(double a,double b,bool valid=true){if(!valid || !std::isfinite(a) || !std::isfinite(b))return;before+=a;after+=b;++count;up+=b-a>.02;down+=b-a<-.02;}
    VerificationResult result(int minimum=3)const{
        VerificationResult r;r.count=count;if(count<minimum)return r;r.before=before/count;r.after=after/count;
        const double change=r.after-r.before;const double agreement=double(change>=0?up:down)/count;
        r.confidence=std::min(1.0,count/5.0)*(std::abs(change)<.02?1.0:agreement);
        r.state=std::abs(change)<.02?5:agreement>=.75?(change>0?3:4):6;return r;
    }
};
inline VerificationResult verifyKickPair(const KickData& before,const KickData& after){
    const auto a=measureKick(before),b=measureKick(after);VerificationAccumulator acc;
    if(!a.valid || !b.valid || std::max(std::abs(a.mean),std::abs(b.mean))<.03)return {};
    // Match reference onset positions explicitly; unsupported events are never guessed.
    for(int i=0;i<a.hits;++i)for(int j=0;j<b.hits;++j)if(a.onsets[i]==b.onsets[j]){acc.add(a.events[i],b.events[j]);break;}
    return acc.result();
}
using SessionStereo=std::array<std::array<float,frameSize>,2>;
struct SourceSectionFrame {SessionStereo a{},b{};Estimate estimate{};};
struct SectionResult {double lag=0,confidence=0,improvement=0,before=0,after=0;bool inverted=false,reliable=false;int count=0,reason=0;std::array<double,maxSections> gains{};};
// Worker-owned capture bank. No allocation or access from the audio callback.
class SectionBank {
    std::array<KickData,maxSections> kicks{};
    std::array<std::vector<SourceSectionFrame>,maxSections> source;
    std::array<LearnedCorrection,maxSections> candidates{};
    int used=0;bool kickMode=true;double rate=48000;
public:
    void clear(){used=0;for(auto& s:source)s.clear();}
    int count()const{return used;}
    void begin(bool kick,double sr){kickMode=kick;rate=sr;}
    void addSourceFrame(const SessionStereo& a,const SessionStereo& b,const Estimate& e){
        if(used>=maxSections || source[used].size()>=64)return;
        source[used].push_back({a,b,e});
    }
    bool finishSource(){
        if(used>=maxSections)return false;
        Learning learning;for(const auto& f:source[used])learning.add(f.estimate);candidates[used]=learning.result(rate,false);++used;return true;
    }
    bool addKick(const KickData& data,const KickResult& result){if(used>=maxSections)return false;kicks[used]=data;candidates[used]={result.lag,result.confidence,result.inverted,result.reliable,result.hits};++used;return true;}
    SectionResult analyse(const KickConfig& config){
        SectionResult out;out.count=used;out.reason=1;if(used<2 || config.search<1 || !std::isfinite(config.currentLag) || !std::isfinite(config.trim) || !std::isfinite(config.grooveLimit))return out;
        for(int i=0;i<used;++i)if(!candidates[i].reliable){out.reason=2;return out;}
        auto actualLag=[&](double lag){const double limit=kickMode?120.0:std::ceil(rate*.02);return std::clamp(lag+config.trim,-limit,limit);};
        auto measure=[&](int section,double lag,bool inverted){
            if(kickMode){const auto m=measureKick(kicks[section],actualLag(lag),inverted!=config.manualInverted);return m.valid?m.mean:std::numeric_limits<double>::quiet_NaN();}
            double sum=0;int valid=0;for(const auto& f:source[section])if(f.estimate.valid){const int ch=strongestPair(f.a[0].data(),f.a[1].data(),f.b[0].data(),f.b[1].data(),frameSize);sum+=predictedCorrelation(f.a[ch].data(),f.b[ch].data(),frameSize,actualLag(lag),inverted!=config.manualInverted);++valid;}
            return valid>=3?sum/valid:std::numeric_limits<double>::quiet_NaN();
        };
        std::array<double,maxSections> baseline{};for(int s=0;s<used;++s){baseline[s]=measure(s,config.currentLag,config.currentInverted);if(!std::isfinite(baseline[s])){out.reason=2;return out;}out.before+=baseline[s]/used;}
        double best=.02;
        for(int c=0;c<used;++c){const double lag=config.allowTiming?candidates[c].lag:config.currentLag;const bool polarity=config.allowPolarity?candidates[c].inverted:config.currentInverted;
            if(config.allowTiming && std::abs(actualLag(lag))>config.search+.0001)continue;
            if(kickMode && config.preserveGroove && std::abs(lag-config.currentLag)>config.grooveLimit+.0001)continue;
            std::array<double,maxSections> gains{};double mean=0,minConfidence=1;int improved=0;bool safe=true;
            for(int s=0;s<used;++s){
                if(kickMode && config.preserveGroove){auto m=measureKick(kicks[s]);const int ch=strongestPair(kicks[s].audio[0].data(),kicks[s].audio[1].data(),kicks[s].audio[2].data(),kicks[s].audio[3].data(),kicks[s].count);for(int h=1;h<m.hits;++h)if(createsGrooveGap(kicks[s].audio[ch].data(),kicks[s].audio[ch+2].data(),kicks[s].count,m.onsets[h-1],m.onsets[h],actualLag(config.currentLag),actualLag(lag)))safe=false;}
                const double value=measure(s,lag,polarity);gains[s]=value-baseline[s];safe=safe && std::isfinite(value) && gains[s]>=-.02;mean+=gains[s]/used;improved+=gains[s]>.01;minConfidence=std::min(minConfidence,candidates[s].confidence);
            }
            if(safe && double(improved)/used>=.75 && mean>best){best=mean;out.lag=lag;out.inverted=polarity;out.improvement=mean;out.after=out.before+mean;out.confidence=minConfidence*double(improved)/used;out.gains=gains;out.reliable=true;out.reason=0;}
        }
        if(!out.reliable)out.reason=3;
        return out;
    }
};
}
