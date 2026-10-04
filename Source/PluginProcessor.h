#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "Ducking.h"
#include "Learning.h"
#include "KickAlignment.h"
#include "ScopeData.h"
#include "ScopePreferences.h"
#include <array>
class PhaseTwinProcessor final : public juce::AudioProcessor, private juce::Thread {
public:
    PhaseTwinProcessor();
    ~PhaseTwinProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "PhaseTwin";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return double(engine.getLatency()+engine.getMaximumLag())/(getSampleRate()>0?getSampleRate():48000.0);}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return {};}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> detectedLag{0},confidence{0},appliedLag{0},residualLag{0},postCorrelation{0};
    std::atomic<bool> verified{false},settling{false},verificationAvailable{false};
    phasetwin::ScopeQueue scope;
    std::atomic<float> duckReductionDb{0};
    std::atomic<float> inputDbA{-100},inputDbB{-100};
    std::atomic<double> hostBpm{120};
    std::atomic<bool> bpmAvailable{false};
    std::atomic<int> analysisReason{int(phasetwin::AnalysisReason::waiting)},analysisChannel{0};
    std::atomic<std::uint32_t> scopePreferences{phasetwin::ScopePreferences{}.packed()},scopeViewRevision{0};
    std::atomic<bool> undoAvailable{false};
    std::atomic<float> priorLagMs{0};
    std::atomic<bool> priorPolarity{false};
    std::atomic<unsigned> cancelRequest{0},undoRequest{0};
    std::atomic<unsigned> alignRequest{0},resetRequest{0},audioBlocks{0};
    std::atomic<float> alignmentScore{0},beforeScore{0},reliability{0},learnProgress{0};
    std::atomic<int> learnState{0}; // 0 idle, 1 learning, 2 applied, 3 unreliable, 4 measured, 5 undone, 6 canceled

    std::atomic<bool> inverted{false},locked{false},referencePresent{false};
private:
    void run() override;
    void commitLearned(double lag,bool polarity);
    void process(juce::AudioBuffer<float>&,bool bypass);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    using StereoFrame=std::array<std::array<float,phasetwin::frameSize>,2>;
    StereoFrame captureA{},captureB{},capturePostA{},capturePostB{};
    StereoFrame jobA{},jobB{},jobPostA{},jobPostB{};
    // 0 = free, 1 = worker owns job, 2 = audio thread may consume result.
    std::atomic<int> mailbox{0};
    phasetwin::Estimate jobEstimate{},jobVerification{};
    double jobPostCorrelation=0,jobBeforeCorrelation=0;
    StereoFrame bandA{},bandB{};
    std::array<float,phasetwin::frameSize> bandPostA{},bandPostB{};
    double jobSampleRate=48000;
    bool jobLowEnd=false;
    float jobCutoff=180;
    phasetwin::KickData kickData;
    phasetwin::KickCapture kickCapture;
    phasetwin::KickAnalyzer kickAnalyzer;
    phasetwin::KickConfig kickConfig;
    phasetwin::KickResult kickResult;
    std::atomic<int> kickMailbox{0};
    std::uint32_t kickGeneration=0;
    bool kickStarted=false;
    phasetwin::Learning learning;
    bool learnActive=false,learnTimedOut=false;
    std::uint64_t learnElapsed=0;
    unsigned seenResetRequest=0,seenCancelRequest=0,seenUndoRequest=0;
    int previousMode=0;
    float previousBand=180;
    bool previousAllowTiming=true,previousAllowPolarity=true;

    int jobSearch=960,jobChannel=0;
    float jobGate=0.65f;
    std::uint32_t generation=0,jobGeneration=0,scopeGeneration=0;
    unsigned seenAlignRequest=0;
    std::uint64_t scopeSampleClock=0;
    std::atomic<std::uint32_t> stateRevision{0};
    std::uint32_t seenStateRevision=0;
    bool previousReference=false,previousBypass=false,previousAuto=true,previousFreeze=false;
    float previousTrim=0,previousPolarity=0,previousGate=0;
    int previousSearch=0;
    double previousLag=0;
    bool previousInvert=false,captureSettled=true,jobWasSettled=false;
    phasetwin::Estimator estimator;
    phasetwin::AudioEngine engine;
    phasetwin::Ducker ducker;
    phasetwin::ScopePacket scopePacket;
    int captureIndex=0,scopeIndex=0;
    double sampleRateHz=48000;
    juce::SmoothedValue<float> referenceMix,outputGain,compareMix;
    std::array<std::atomic<float>*,17> values{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseTwinProcessor)
};
