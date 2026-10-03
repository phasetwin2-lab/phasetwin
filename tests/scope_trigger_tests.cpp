#include "TriggeredScope.h"
#include <iostream>
#include <cstdlib>
using namespace phasetwin;
void check(bool value,const char* name){if(!value){std::cerr<<"FAIL "<<name<<'\n';std::exit(1);}std::cout<<"PASS "<<name<<'\n';}
int main(){
 check(scopeDivisionBeats(5)==1,"quarter-note division");check(scopeDivisionBeats(7)==4,"whole-note division");
 check(scopeDurationMs(true,0,scopeDivisionBeats(7),120,true)==2000,"whole note at 120 BPM is 2000 ms");
 check(samplesToMs(96,48000)==2,"delay samples to milliseconds");check(samplesToMs(-192,96000)==-2,"signed delay conversion");
 TriggeredScope scope;scope.setWindow(20);ScopePacket p;p.sampleRate=48000;p.generation=1;
 auto feed=[&](int packets,bool kick){for(int k=0;k<packets;++k){for(auto& trace:p.traces)trace.fill(0);if(kick)for(int n=0;n<scopePacketSamples;++n){float x=float(.5*std::sin(2*3.141592653589793*60*(k*scopePacketSamples+n)/48000.0)*std::exp(-double(k*scopePacketSamples+n)/1200));for(int c=0;c<2;++c){p.traces[2+c][n]=x;p.traces[c][n]=-.5f*x;p.traces[4+c][n]=.5f*x;p.traces[6+c][n]=x;}}scope.ingest(p);p.firstSample+=scopePacketSamples;}};
 feed(1,true);check(scope.publishedFrames()==0,"partial capture is not displayed");feed(10,true);check(scope.publishedFrames()==1,"complete kick capture published");
 const auto r=scope.history().range(1,0,0,960,960);check(r.second>.1f,"kick visible in snapshot");
 feed(40,false);check(scope.publishedFrames()==1,"silence retains snapshot");check(scope.history().range(1,0,0,960,960)==r,"held frame does not move");
 feed(12,true);check(scope.publishedFrames()==2,"next kick refreshes snapshot");
 p.firstSample+=1024;feed(1,false);check(scope.publishedFrames()==2,"packet gap does not publish partial frame");
 p.generation=2;feed(1,false);check(scope.history().availableSamples(20)==0,"routing generation clears obsolete frame");
 scope.setWindow(500);check(scope.displayedWindowMs()==20,"new division retains old capture duration until replacement");
 auto longScope=std::make_unique<TriggeredScope>();longScope->setWindow(4000);ScopePacket longPacket;longPacket.sampleRate=192000;
 for(int k=0;k<3000;++k){for(auto& trace:longPacket.traces)trace.fill(0);if(k==0)longPacket.traces[2].fill(.5f);longScope->ingest(longPacket);longPacket.firstSample+=scopePacketSamples;}
 check(longScope->publishedFrames()==1,"long capture publishes after exact requested duration");
 longScope->ingest(longPacket);check(longScope->publishedFrames()==1,"four-second stable capture at 192 kHz");check(longScope->history().availableSamples(4000)>=767984,"full long-duration snapshot retained");
 ScopeHistory sliced;ScopePacket sample;sample.firstSample=0;sample.sampleRate=48000;sample.traces[0][10]=1;sliced.ingest(sample,10,20);check(sliced.availableSamples(1)==20,"sample-exact sliced capture length");check(sliced.range(0,0,0,1,20).second==1,"slice starts at exact trigger sample");
}
