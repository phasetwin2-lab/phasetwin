#pragma once
#include "ScopeHistory.h"
#include <memory>
namespace phasetwin {
// GUI-thread-only: publish complete captures; never scroll an unfinished frame.
class TriggeredScope {
    std::unique_ptr<ScopeHistory> shown=std::make_unique<ScopeHistory>(),building=std::make_unique<ScopeHistory>();
    double completedWindow=500;
    double window=500,rate=48000,envelope=0,peak=0;
    std::uint64_t next=0;
    std::uint32_t generation=0;
    int captured=0,target=24000,refractory=0,channel=0;
    bool initialized=false,collecting=false,armed=true;
    unsigned frames=0;
public:
    const ScopeHistory& history()const{return *shown;}
    double displayedWindowMs()const{return completedWindow;}
    unsigned publishedFrames()const{return frames;}
    void cancel(){collecting=false;captured=0;building->clear();}
    void setChannel(int c){if(c!=channel){channel=std::clamp(c,0,2);cancel();shown->clear();}}
    void setWindow(double ms){ms=std::clamp(ms,2.0,4000.0);if(std::abs(ms-window)>0.01){window=ms;cancel();}}
    void ingest(const ScopePacket& p){
        if(!initialized || p.sampleRate!=rate || p.generation!=generation){cancel();shown->clear();envelope=peak=0;armed=true;refractory=0;}
        else if(p.firstSample!=next){cancel();armed=true;}
        initialized=true;rate=p.sampleRate;generation=p.generation;next=p.firstSample+scopePacketSamples;
        target=std::max(2,int(std::ceil(rate*window/1000)));
        const double attack=1-std::exp(-1/(rate*.003)),decay=std::exp(-1/(rate*1.0));
        for(int n=0;n<scopePacketSamples;++n){
            double x=channel==2?0.5*(p.traces[2][n]+p.traces[3][n]):p.traces[2+channel][n];
            envelope+=attack*(std::abs(x)-envelope);peak=std::max(envelope,peak*decay);
            if(refractory>0)--refractory;
            if(envelope<std::max(1e-6,peak*.12))armed=true;
            if(!collecting && armed && refractory==0 && envelope>std::max(1e-5,peak*.3)){
                collecting=true;captured=0;building->clear();armed=false;refractory=int(rate*.125);
            }
            if(collecting){building->ingest(p,n,1);if(++captured>=target){shown.swap(building);completedWindow=1000.0*captured/rate;collecting=false;++frames;}}
        }
    }
};
}
