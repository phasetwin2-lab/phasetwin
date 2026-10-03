#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
namespace phasetwin {
struct ScopePreferences {
    int style=1,mode=0,channel=0,division=5;
    bool sync=true,showA=true,showB=true,summed=false;
    double milliseconds=125;
    std::uint32_t packed()const{
        const int ms=std::isfinite(milliseconds)?std::clamp(int(std::round(std::clamp(milliseconds,2.0,4000.0))),2,4000):125;
        return std::uint32_t(std::clamp(style,0,1) | (std::clamp(mode,0,1)<<1) | (std::clamp(channel,0,2)<<2) | (std::clamp(division,1,7)<<4) | (int(sync)<<7) | (ms<<8) | (int(showA)<<20) | (int(showB)<<21) | (int(summed)<<22));
    }
    static ScopePreferences unpack(std::uint32_t value){ScopePreferences p;p.style=value&1;p.mode=(value>>1)&1;p.channel=std::min(2,int((value>>2)&3));p.division=std::clamp(int((value>>4)&7),1,7);p.sync=((value>>7)&1)!=0;p.milliseconds=std::clamp(int((value>>8)&4095),2,4000);p.showA=((value>>20)&1)!=0;p.showB=((value>>21)&1)!=0;p.summed=((value>>22)&1)!=0;return p;}
};
// Filled display retains both signed lobes and min/max envelope peaks.
inline std::pair<float,float> filledBounds(float low,float high){return {std::min(0.0f,low),std::max(0.0f,high)};}
}
