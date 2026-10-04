#pragma once
#include "Alignment.h"
#include <array>
namespace phasetwin {
struct AlignedSample { std::array<float, 2> a{}, b{}, neutralA{}; };
// Used by both the JUCE wrapper and the standalone end-to-end DSP tests.
class AudioEngine {
    std::array<Delay, 2> target, reference;
    double sr = 48000, delay = 960, sign = 1, desiredDelay = 960, desiredSign = 1;
    int latency = 992,maximumLag=960,transitionSamples=960,transitionPosition=0;
    double nextDelay=960,nextSign=1,blend=0;
    bool transitioning=false;
public:
    void prepare(double sampleRate, double lag, bool invert) {
        sr = sampleRate;
        maximumLag = int(std::ceil(sr * 0.020));
        latency=maximumLag+32;
        for (auto& d : target) d.prepare(latency * 2 + 64);
        for (auto& d : reference) d.prepare(latency * 2 + 64);
        setCorrection(lag, invert);
        delay = desiredDelay; sign = desiredSign;nextDelay=delay;nextSign=sign;blend=0;transitionPosition=0;transitioning=false;transitionSamples=std::max(1,int(std::ceil(sr*.02)));
    }
    void setCorrection(double lag, bool invert) noexcept {
        desiredDelay = latency - std::clamp(lag, -double(maximumLag), double(maximumLag));
        desiredSign = invert ? -1.0 : 1.0;
    }
    int getMaximumLag()const noexcept{return maximumLag;}
    int getLatency() const noexcept { return latency; }
    double appliedLag() const noexcept { return latency-(transitioning?delay*(1-blend)+nextDelay*blend:delay); }
    bool isSettling() const noexcept {
        return transitioning || std::abs(delay - desiredDelay) > 0.01 || std::abs(sign - desiredSign) > 0.001;
    }
    AlignedSample process(const std::array<float, 2>& a, const std::array<float, 2>& b) noexcept {
        if(!transitioning && (std::abs(desiredDelay-delay)>1e-9 || desiredSign!=sign)){
            nextDelay=desiredDelay;nextSign=desiredSign;blend=0;transitionPosition=0;transitioning=true;
        }
        if(transitioning)blend=std::min(1.0,double(++transitionPosition)/transitionSamples);
        AlignedSample result;
        for (int ch = 0; ch < 2; ++ch) {
            target[ch].push(a[ch]); reference[ch].push(b[ch]);
            result.a[ch] = float(sign) * target[ch].readBandlimited(delay);
            if(transitioning)result.a[ch]=float(result.a[ch]*(1-blend)+nextSign*target[ch].readBandlimited(nextDelay)*blend);
            result.b[ch] = reference[ch].read(latency);
            result.neutralA[ch]=target[ch].read(latency);
        }
        if(transitioning && blend>=1){delay=nextDelay;sign=nextSign;transitioning=false;}
        return result;
    }
};
// Signed zero-lag correlation of the actual processed signals, with DC removed.
inline double zeroLagCorrelation(const float* a, const float* b, int count) {
    if(count<=0)return 0;
    double aa=0, bb=0, ab=0, ma=0, mb=0;
    for (int i=0; i<count; ++i) {
        if (!std::isfinite(a[i]) || !std::isfinite(b[i])) return 0;
        ma+=a[i]; mb+=b[i];
    }
    ma/=count; mb/=count;
    for (int i=0; i<count; ++i) {
        double x=a[i]-ma, y=b[i]-mb; aa+=x*x; bb+=y*y; ab+=x*y;
    }
    if (aa/count<1e-9 || bb/count<1e-9) return 0;
    return std::clamp(ab/std::sqrt(aa*bb), -1.0, 1.0);
}
}

namespace phasetwin {
// Predicted correlation on captured input, using linear fractional sampling.
// A candidate measurement, not a claim of live output verification.
inline double predictedCorrelation(const float* a,const float* b,int count,double lag,bool invert){
    if(count<2 || !std::isfinite(lag) || std::abs(lag)>=count)return 0;
    double sx=0,sy=0,sxx=0,syy=0,sxy=0;int used=0;
    for(int n=0;n<count;++n){const double p=n+lag;const int i=int(std::floor(p));if(i<0 || i+1>=count)continue;const double x=(double(a[i])+(p-i)*(a[i+1]-a[i]))*(invert?-1:1),y=b[n];sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;++used;}
    if(used<2)return 0;
    const double xx=sxx-sx*sx/used,yy=syy-sy*sy/used;return xx>1e-12 && yy>1e-12?std::clamp((sxy-sx*sy/used)/std::sqrt(xx*yy),-1.0,1.0):0;
}
}
