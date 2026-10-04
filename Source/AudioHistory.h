#pragma once
#include <array>
#include <cstddef>
namespace phasetwin {
// Message-thread history only. Audio processing never allocates or locks it.
template<class State, std::size_t Steps=128> class AudioHistory {
public:
    void reset(const State& state){states[0]=state;position=last=0;}
    bool push(const State& state){
        if(state==states[position])return false;
        last=position; // A new edit discards the redo branch.
        if(last==Steps){for(std::size_t i=1;i<=Steps;++i)states[i-1]=states[i];--last;--position;}
        states[++position]=state;last=position;return true;
    }
    // Continuous tracking is one evolving step until another user action.
    void replaceTip(const State& state){if(position==last && position>0)states[position]=state;else push(state);}
    const State* undo(){return position?&states[--position]:nullptr;}
    const State* redo(){return position<last?&states[++position]:nullptr;}
    std::size_t undoCount()const{return position;}
    std::size_t redoCount()const{return last-position;}
    const State& current()const{return states[position];}
private:
    std::array<State,Steps+1> states{};
    std::size_t position=0,last=0;
};
struct AudioSettings {
    std::array<float,30> parameters{}; // Normalized host parameter values.
    float lagMs=0;bool polarity=false;
    bool operator==(const AudioSettings& other)const{return parameters==other.parameters && lagMs==other.lagMs && polarity==other.polarity;}
    bool operator!=(const AudioSettings& other)const{return !(*this==other);}
};
inline bool historyParameter(std::size_t index){return index!=9 && index!=10 && index!=19 && index!=28 && index!=29;}
}
