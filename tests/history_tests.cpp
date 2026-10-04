#include "AudioHistory.h"
#include <iostream>
#include <stdexcept>
static int checks=0;
static void require(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
int main(){try{
    phasetwin::AudioHistory<phasetwin::AudioSettings,4> h;phasetwin::AudioSettings s;h.reset(s);
    require(!h.undo() && !h.redo(),"empty history");require(!h.push(s),"no-op became edit");
    for(int i=1;i<=4;++i){s.parameters[7]=float(i)/10;require(h.push(s),"gain edit not recorded");}
    for(int i=3;i>=0;--i){auto* p=h.undo();require(p && p->parameters[7]==float(i)/10,"wrong undo order");}
    require(!h.undo(),"undid beyond baseline");
    for(int i=1;i<=4;++i){auto* p=h.redo();require(p && p->parameters[7]==float(i)/10,"wrong redo order");}
    require(!h.redo(),"redid beyond tip");
    h.undo();h.undo();s=h.current();s.lagMs=1.2345f;s.polarity=true;h.push(s);require(h.redoCount()==0,"new edit kept redo branch");
    const auto applied=s;require(h.undo()->lagMs==0,"fractional correction undo");require(*h.redo()==applied,"fractional correction redo");
    s.parameters[14]=.9f;h.push(s);auto reset=s;reset.lagMs=0;reset.polarity=false;reset.parameters[4]=.5f;reset.parameters[5]=0;h.push(reset);require(*h.undo()==s,"reset not reversible");require(*h.redo()==reset,"reset redo");
    // Full history eviction retains the latest Steps reversible edits.
    h.reset({});for(int i=1;i<=20;++i){s={};s.lagMs=float(i);h.push(s);}require(h.undoCount()==4,"capacity bound");for(int i=19;i>=16;--i)require(h.undo()->lagMs==i,"eviction order");require(!h.undo(),"capacity exceeded");
    h.reset({});s={};s.lagMs=1;h.push(s);for(int i=2;i<100;++i){s.lagMs=float(i);h.replaceTip(s);}require(h.undoCount()==1 && h.undo()->lagMs==0,"continuous tracking flooded history");require(h.redo()->lagMs==99,"continuous tracking redo");
    h.reset({});s={};for(std::size_t i=0;i<30;++i)if(phasetwin::historyParameter(i)){s.parameters[i]=.25f;h.push(s);auto* old=h.undo();require(old && old->parameters[i]==0,"audio parameter undo");require(h.redo()->parameters[i]==.25f,"audio parameter redo");}
    require(!phasetwin::historyParameter(28) && !phasetwin::historyParameter(29),"analysis workflow in audio history");
    require(!phasetwin::historyParameter(19),"workflow preview in audio history");require(!phasetwin::historyParameter(9) && !phasetwin::historyParameter(10),"legacy parameters in history");
    std::cout<<checks<<" history checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
