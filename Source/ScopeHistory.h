#pragma once
#include "ScopeData.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace phasetwin {
inline double scopeDurationMs(bool sync,double requestedMs,double beats,double bpm,bool bpmAvailable){
    const double tempo=bpmAvailable && std::isfinite(bpm) && bpm>0?bpm:120.0;
    return std::clamp(sync?60000.0*beats/tempo:requestedMs,2.0,4000.0);
}
inline double samplesToMs(double samples,double rate){return std::isfinite(rate) && rate>0?1000.0*samples/rate:0.0;}
inline double scopeDivisionBeats(int id){return std::pow(2.0,std::clamp(id,1,7)-1)/16.0;}
// GUI-thread-only two-resolution history. Longer views retain min/max peaks.
class ScopeHistory {
    static constexpr int rawCapacity=16384,envelopeCapacity=65536,decimation=16,traces=27;
    std::vector<float> raw,low,high;
    std::array<float,traces> pendingLow{},pendingHigh{};
    int rawHead=0,rawCount=0,envHead=0,envCount=0,pendingCount=0;
    std::uint64_t nextSequence=0;
    std::uint32_t generation=0;
    bool initialized=false;
    double rate=48000;
    float rawAt(int trace,int offset)const{return raw[trace*rawCapacity+(rawHead+rawCapacity-rawCount+offset)%rawCapacity];}
public:
    ScopeHistory():raw(traces*rawCapacity),low(traces*envelopeCapacity),high(traces*envelopeCapacity){}
    void clear(){rawHead=rawCount=envHead=envCount=pendingCount=0;initialized=false;}
    double sampleRate()const{return rate;}
    void ingest(const ScopePacket& p,int begin=0,int length=scopePacketSamples){
        if(begin<0 || length<1 || begin+length>scopePacketSamples)return;
        if(!initialized || p.generation!=generation || p.sampleRate!=rate || p.firstSample+begin!=nextSequence)clear();
        generation=p.generation;rate=p.sampleRate;nextSequence=p.firstSample+begin+length;initialized=true;
        for(int n=begin;n<begin+length;++n){
            for(int wave=0;wave<9;++wave)for(int channel=0;channel<3;++channel){
                const int t=wave*3+channel;
                auto value=[&](int ch){if(wave<4)return p.traces[wave*2+ch][n];if(wave==6)return p.traces[8+ch][n];if(wave==7)return p.traces[8+ch][n]+p.traces[6+ch][n];if(wave==8)return p.reductionDb[n];const int base=wave==4?0:4;return p.traces[base+ch][n]+p.traces[base+2+ch][n];};
                const float v=channel==2?0.5f*(value(0)+value(1)):value(channel);
                raw[t*rawCapacity+rawHead]=v;
                if(pendingCount==0)pendingLow[t]=pendingHigh[t]=v;
                else{pendingLow[t]=std::min(pendingLow[t],v);pendingHigh[t]=std::max(pendingHigh[t],v);}
            }
            rawHead=(rawHead+1)%rawCapacity;rawCount=std::min(rawCount+1,rawCapacity);
            if(++pendingCount==decimation){
                pendingCount=0;
                for(int t=0;t<traces;++t){low[t*envelopeCapacity+envHead]=pendingLow[t];high[t*envelopeCapacity+envHead]=pendingHigh[t];}
                envHead=(envHead+1)%envelopeCapacity;envCount=std::min(envCount+1,envelopeCapacity);
            }
        }
    }
    int availableSamples(double ms)const{
        int desired=std::max(2,int(rate*ms/1000));
        return std::min(desired,desired<=rawCapacity?rawCount:envCount*decimation);
    }
    std::pair<float,float> range(int wave,int channel,int start,int end,int count)const{
        const int trace=wave*3+channel;
        float lo=1e20f,hi=-1e20f;
        if(count<=rawCapacity){
            for(int n=start;n<end;++n){float v=rawAt(trace,rawCount-count+n);lo=std::min(lo,v);hi=std::max(hi,v);}
        }else{
            int first=std::max(0,(envCount*decimation-count+start)/decimation);
            int last=std::min(envCount,(envCount*decimation-count+end+decimation-1)/decimation);
            for(int n=first;n<last;++n){int index=trace*envelopeCapacity+(envHead+envelopeCapacity-envCount+n)%envelopeCapacity;lo=std::min(lo,low[index]);hi=std::max(hi,high[index]);}
        }
        return lo<=hi?std::make_pair(lo,hi):std::make_pair(0.f,0.f);
    }
};
}
