#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace phasetwin {
// Stereo-linked amplitude ducking; independent of alignment analysis.
class Ducker {
    double rate=48000,envelope=0,gain=1,depth=0,desiredDepth=0;
    double attack=0,release=0,controlSmooth=0,gainSmooth=0;
    float lastHarshness=-1;
    double lastAttack=-1,lastRelease=-1,sensitivity=1,desiredSensitivity=1;
public:
    void prepare(double sr){rate=std::isfinite(sr)&&sr>=8000?sr:48000;envelope=depth=desiredDepth=0;gain=1;lastHarshness=-1;lastAttack=lastRelease=-1;sensitivity=desiredSensitivity=1;controlSmooth=1-std::exp(-1/(rate*.005));gainSmooth=1-std::exp(-1/(rate*.001));configure(false,0,50);}
    void configure(bool enabled,float amountPercent,float harshnessPercent,bool advanced=false,float attackMs=3,float releaseMs=125,float sensitivityDb=0){
        const double amount=std::isfinite(amountPercent)?std::clamp(double(amountPercent),0.0,100.0)/100:0;
        const float harshness=std::isfinite(harshnessPercent)?std::clamp(harshnessPercent,0.0f,100.0f):50;
        desiredDepth=enabled?amount:0;
        lastHarshness=harshness;const double h=harshness/100.0;
        const double a=advanced?(std::isfinite(attackMs)?std::clamp(double(attackMs),.1,100.0):3):15*std::pow(.3/15,h);
        const double r=advanced?(std::isfinite(releaseMs)?std::clamp(double(releaseMs),10.0,1000.0):125):250*std::pow(60.0/250,h);
        if(a!=lastAttack){lastAttack=a;attack=1-std::exp(-1/(rate*a*.001));}
        if(r!=lastRelease){lastRelease=r;release=1-std::exp(-1/(rate*r*.001));}
        desiredSensitivity=std::pow(10.0,(std::isfinite(sensitivityDb)?std::clamp(double(sensitivityDb),-24.0,24.0):0)/20);

    }
    float process(const std::array<float,2>& reference) noexcept {
        double level=0;for(float x:reference)if(std::isfinite(x))level=std::max(level,std::abs(double(x)));
        sensitivity+=controlSmooth*(desiredSensitivity-sensitivity);level*=sensitivity;
        envelope+=(level>envelope?attack:release)*(level-envelope);depth+=controlSmooth*(desiredDepth-depth);
        if(desiredDepth==0 && depth==0 && gain==1)return 1;
        // Fixed detector region: -36 to -12 dBFS; full activity reaches the
        // selected maximum depth. Sidechain level therefore matters.
        double activity=std::clamp((envelope-.0158489319)/(.2511886432-.0158489319),0.0,1.0);
        const double h=lastHarshness/100.0;
        activity=(1-h)*activity+h*activity*activity*(3-2*activity);
        const double desiredGain=std::exp(-.11512925464970229*24*depth*activity);
        gain+=gainSmooth*(desiredGain-gain);
        if(desiredDepth==0 && depth<1e-9 && 1-gain<1e-9){depth=0;gain=1;}
        return float(std::clamp(gain,0.0,1.0));
    }
    float reductionDb()const noexcept{return float(-20*std::log10(std::max(1e-12,gain)));}
};
}
