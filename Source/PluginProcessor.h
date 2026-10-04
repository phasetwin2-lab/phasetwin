#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "AudioHistory.h"
#include "SessionAnalysis.h"
#include "Ducking.h"
#include "PhaseTools.h"
#include "Learning.h"
#include "KickAlignment.h"
#include "ScopeData.h"
#include "ScopePreferences.h"
#include <array>
class PhaseTwinProcessor final : public juce::AudioProcessor, private juce::Thread, private juce::Timer, private juce::AudioProcessorParameter::Listener {
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
    double getTailLengthSeconds() const override {return .3+double(engine.getLatency()+engine.getMaximumLag())/(getSampleRate()>0?getSampleRate():48000.0);}
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
    std::atomic<bool> hearProposalRequested{false},hearingProposal{false};
    std::atomic<bool> recommendationReady{false};
    std::atomic<float> proposedLagMs{0},proposedGain{0};
    std::atomic<bool> proposedPolarity{false};
    std::atomic<int> recommendationReason{0};
    std::atomic<unsigned> applyRequest{0};
    std::atomic<int> verifyState{0},verifyCount{0},sectionCount{0};
    std::atomic<float> verifyBefore{0},verifyAfter{0},verifyConfidence{0},verifyProgress{0};
    std::array<std::atomic<float>,phasetwin::maxSections> sectionGains{};
    std::atomic<unsigned> evaluateSectionsRequest{0},clearSectionsRequest{0};
    std::array<std::atomic<float>,phasetwin::spectralBands> spectralBefore{},spectralAfter{},spectralConfidence{},spectralLevelDb{};
    std::atomic<unsigned> spectralEpoch{0};std::atomic<double> spectralSampleRate{48000};
    std::atomic<unsigned> editorSections{0};
    std::atomic<bool> spectrumAvailable{false};
    phasetwin::SpectralAnalyzer spectralAnalyzer;phasetwin::SpectralFrame jobSpectrum;
    phasetwin::PhaseRotator rotator;
    float previousRotationFrequency=80,previousRotationQ=.707f;bool previousRotation=false;
    std::atomic<float> outputPeakL{-100},outputPeakR{-100};
    std::atomic<bool> outputClipped{false};
    std::atomic<unsigned> clearClipRequest{0};
    unsigned seenClearClip=0;
    std::atomic<float> duckReductionDb{0};
    std::atomic<float> inputDbA{-100},inputDbB{-100};
    std::atomic<double> hostBpm{120};
    std::atomic<bool> bpmAvailable{false};
    std::atomic<int> analysisReason{int(phasetwin::AnalysisReason::waiting)},analysisChannel{0};
    std::atomic<std::uint32_t> scopePreferences{phasetwin::ScopePreferences{}.packed()},scopeViewRevision{0};
    std::atomic<bool> undoAvailable{false};
    std::atomic<bool> redoAvailable{false};
    std::atomic<unsigned> undoSteps{0},redoSteps{0};
    std::atomic<unsigned> cancelRequest{0};
    void captureHistory(bool force=true); // Message thread; also used by deterministic native tests.
    void beginHistoryAction();
    void endHistoryAction();
    void undoAudio();
    void redoAudio();
    bool historyNavigationReady()const{return historyGestures.load()==0 && resetRequest.load()==resetAcknowledged.load();}
    std::atomic<unsigned> alignRequest{0},resetRequest{0},audioBlocks{0};
    std::atomic<float> alignmentScore{0},beforeScore{0},reliability{0},learnProgress{0};
    std::atomic<int> learnState{0}; // 0 idle, 1 learning, 2 applied, 3 unreliable, 4 measured, 5 undone, 6 canceled, 7 preview, 8 no useful correction, 9 section collected, 10 evaluating sections

    std::atomic<bool> inverted{false},locked{false},referencePresent{false};
private:
    void timerCallback() override {captureHistory(false);}
    void parameterValueChanged(int,float) override;
    void parameterGestureChanged(int,bool) override;
    phasetwin::AudioSettings readAudioSettings() const;
    void restoreAudioSettings(const phasetwin::AudioSettings&);
    void updateHistoryAvailability();
    void publishHeldCorrection();
    phasetwin::AudioHistory<phasetwin::AudioSettings> history;
    std::array<juce::RangedAudioParameter*,30> historyParameters{};
    std::atomic<int> historyGestures{0};
    std::array<std::atomic<bool>,30> activeHistoryGestures{};
    std::atomic<juce::uint32> lastAutomationChange{0};
    std::atomic<unsigned> ignoreResetRequest{0};
    int historyActionDepth=0;
    std::atomic<bool> historySuppressed{false},historyBaselineNeeded{false};
    bool trackingHistoryStep=false;
    std::atomic<std::uint64_t> heldCorrectionPacked{0};
    std::atomic<std::uint64_t> restoreCorrectionPacked{0};
    std::atomic<unsigned> restoreRequest{0},restoreAcknowledged{0},resetAcknowledged{0};
    unsigned seenRestoreRequest=0;
    void clearSectionSession();
    void syncSectionBank(); // Worker thread only.
    void startVerification();
    void publishVerification(const phasetwin::VerificationResult&);
    std::unique_ptr<phasetwin::SectionBank> sectionBank=std::make_unique<phasetwin::SectionBank>();
    std::atomic<unsigned> sectionEpoch{0};unsigned workerSectionEpoch=0;
    unsigned seenEvaluateSections=0,seenClearSections=0;
    bool previousMulti=false,previousVerify=true;
    bool kickJobMulti=false,jobSectionCollect=false;
    unsigned kickSectionEpoch=0,jobSectionEpoch=0;
    int kickSectionCount=0;
    std::atomic<int> sectionMailbox{0};
    int sectionCommand=0;unsigned sectionCommandEpoch=0;std::uint32_t sectionCommandGeneration=0;
    phasetwin::KickConfig sectionConfig;
    phasetwin::SectionResult sectionResult;
    phasetwin::KickData verifyKickBefore,verifyKickAfter;
    phasetwin::KickCapture verifyCaptureBefore,verifyCaptureAfter;
    std::atomic<int> checkMailbox{0};
    phasetwin::VerificationResult checkResult;
    unsigned verifyEpoch=0,checkEpoch=0,jobVerifyEpoch=0;
    bool verifyActive=false,verifyStarted=false;
    std::uint64_t verifyWait=0,verifyElapsed=0;
    phasetwin::VerificationAccumulator verifyAccumulator;
    void run() override;
    void propose(double lag,bool polarity,double improvement);
    unsigned seenApplyRequest=0;
    std::uint32_t recommendationGeneration=0;
    bool previousGroove=true,previousPreview=true;
    float previousGrooveLimit=2;
    void commitLearned(double lag,bool polarity);
    void process(juce::AudioBuffer<float>&,bool bypass);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    using StereoFrame=std::array<std::array<float,phasetwin::frameSize>,2>;
    StereoFrame captureA{},captureB{},capturePostA{},capturePostB{},captureNeutralA{};
    StereoFrame jobA{},jobB{},jobPostA{},jobPostB{},jobNeutralA{};
    // 0 = free, 1 = worker owns job, 2 = audio thread may consume result.
    std::atomic<int> mailbox{0};
    phasetwin::Estimate jobEstimate{},jobVerification{};
    double jobPostCorrelation=0,jobBeforeCorrelation=0,jobNeutralCorrelation=0;
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
    unsigned seenResetRequest=0,seenCancelRequest=0;
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
    std::array<std::atomic<float>*,30> values{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseTwinProcessor)
};
