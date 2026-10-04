#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>
juce::AudioProcessorValueTreeState::ParameterLayout PhaseTwinProcessor::layout(){
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"auto",1},"Continuous tracking",false));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"freeze",1},"Freeze correction",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"range",1},"Search range (ms)",juce::NormalisableRange<float>(1,20,0.1f),20));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"confidence",1},"Minimum correlation",juce::NormalisableRange<float>(0.15f,0.99f,0.01f),0.65f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"manual",1},"Manual lag / auto trim (ms)",juce::NormalisableRange<float>(-20,20,0.001f),0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"polarity",1},"Manual polarity override",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix",1},"Add reference B to output",juce::NormalisableRange<float>(0,1,0.01f),0));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain",1},"Output gain (dB)",juce::NormalisableRange<float>(-24,6,0.1f),0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"profile",1},"Analysis profile",juce::StringArray{"Same source","Kick / bass focus"},1));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"allowDelay",1},"Legacy timing permission (state migration only)",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"allowPolarity",1},"Legacy polarity permission (state migration only)",true));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"band",1},"Low-end focus (Hz)",juce::NormalisableRange<float>(50,500,1),180));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"compare",1},"Compare neutral",false));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"duckEnabled",1},"Sidechain ducking",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duckAmount",1},"Ducking amount (%)",juce::NormalisableRange<float>(0,100,1),50));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duckHarshness",1},"Ducking harshness (%)",juce::NormalisableRange<float>(0,100,1),50));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"correctionMode",1},"Automatic correction",juce::StringArray{"Timing + polarity","Preserve polarity","Preserve timing"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"preserveGroove",1},"Preserve kick/bass groove",true));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"grooveLimit",1},"Groove shift limit (ms)",juce::NormalisableRange<float>(.1f,5,.1f),2));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"preview",1},"Preview before apply",true));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"audition",1},"Correction audition",juce::StringArray{"Timing + polarity","Timing only","Polarity only","Neither"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"duckAdvanced",1},"Advanced ducking envelope",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duckAttack",1},"Ducking attack (ms)",juce::NormalisableRange<float>(.1f,100,.1f,.4f),3));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duckRelease",1},"Ducking release (ms)",juce::NormalisableRange<float>(10,1000,1,.4f),125));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"duckSensitivity",1},"Ducking detector sensitivity (dB)",juce::NormalisableRange<float>(-24,24,.1f),0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"rotation",1},"All-pass phase rotation",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rotationFrequency",1},"All-pass centre (Hz)",juce::NormalisableRange<float>(20,2000,1,.35f),80));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"rotationQ",1},"All-pass Q",juce::NormalisableRange<float>(.2f,2,.001f),.707f));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"verifyAfterApply",1},"Verify after apply",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"multiSection",1},"Collect multiple sections",false));
    return p;
}
PhaseTwinProcessor::PhaseTwinProcessor():AudioProcessor(BusesProperties().withInput("Target A",juce::AudioChannelSet::stereo(),true).withInput("Reference B",juce::AudioChannelSet::stereo(),true).withOutput("Aligned output",juce::AudioChannelSet::stereo(),true)),Thread("PhaseTwin analysis"),parameters(*this,nullptr,"PhaseTwin",layout()){
    const char* ids[]={"auto","freeze","range","confidence","manual","polarity","mix","gain","profile","allowDelay","allowPolarity","band","compare","duckEnabled","duckAmount","duckHarshness","correctionMode","preserveGroove","grooveLimit","preview","audition","duckAdvanced","duckAttack","duckRelease","duckSensitivity","rotation","rotationFrequency","rotationQ","verifyAfterApply","multiSection"};
    for(size_t i=0;i<values.size();++i){values[i]=parameters.getRawParameterValue(ids[i]);historyParameters[i]=parameters.getParameter(ids[i]);historyParameters[i]->addListener(this);}
    history.reset(readAudioSettings());startTimerHz(30);
}
PhaseTwinProcessor::~PhaseTwinProcessor(){stopTimer();for(auto* parameter:historyParameters)parameter->removeListener(this);stopThread(-1);}
namespace {
bool onMessageThread(){auto* manager=juce::MessageManager::getInstanceWithoutCreating();return manager && manager->isThisTheMessageThread();}
}
void PhaseTwinProcessor::publishHeldCorrection(){
    const float ms=float(detectedLag.load()*1000/sampleRateHz);std::uint32_t bits=0;std::memcpy(&bits,&ms,sizeof(bits));
    heldCorrectionPacked.store(std::uint64_t(bits) | (std::uint64_t(inverted.load())<<32),std::memory_order_release);
}
phasetwin::AudioSettings PhaseTwinProcessor::readAudioSettings()const{
    phasetwin::AudioSettings state;
    for(size_t i=0;i<historyParameters.size();++i)if(phasetwin::historyParameter(i))state.parameters[i]=historyParameters[i]->getValue();
    const auto packed=heldCorrectionPacked.load(std::memory_order_acquire);const auto bits=std::uint32_t(packed);std::memcpy(&state.lagMs,&bits,sizeof(bits));state.polarity=(packed>>32)!=0;
    return state;
}
void PhaseTwinProcessor::updateHistoryAvailability(){undoSteps=unsigned(history.undoCount());redoSteps=unsigned(history.redoCount());undoAvailable=history.undoCount()!=0;redoAvailable=history.redoCount()!=0;}
void PhaseTwinProcessor::captureHistory(bool force){
    jassert(onMessageThread());
    if(historySuppressed.load() || historyGestures.load()>0 || historyActionDepth>0)return;
    if(!force && juce::uint32(juce::Time::getMillisecondCounter()-lastAutomationChange.load())<250)return;
    if(resetAcknowledged.load()!=resetRequest.load())return;
    auto state=readAudioSettings();
    if(restoreAcknowledged.load()!=restoreRequest.load()){
        const auto packed=restoreCorrectionPacked.load();const auto bits=std::uint32_t(packed);std::memcpy(&state.lagMs,&bits,sizeof(bits));state.polarity=(packed>>32)!=0;
    }
    if(historyBaselineNeeded.exchange(false)){history.reset(state);trackingHistoryStep=false;}
    else if(state!=history.current()){
        const bool onlyTracking=state.parameters==history.current().parameters && state.parameters[0]>.5f;
        if(onlyTracking && trackingHistoryStep)history.replaceTip(state);
        else{history.push(state);trackingHistoryStep=onlyTracking;}
    }
    updateHistoryAvailability();
}
void PhaseTwinProcessor::parameterValueChanged(int index,float){
    if(index<0 || !phasetwin::historyParameter(size_t(index)) || historySuppressed.load())return;
    if(onMessageThread())captureHistory();
    else lastAutomationChange=juce::Time::getMillisecondCounter(); // Coalesce ungestured automation until quiet, or explicit Undo.
}
void PhaseTwinProcessor::parameterGestureChanged(int index,bool starting){
    if(index<0 || size_t(index)>=activeHistoryGestures.size() || !phasetwin::historyParameter(size_t(index)))return;
    const bool canCapture=onMessageThread() && !historySuppressed.load();
    if(starting){if(canCapture)captureHistory();if(!activeHistoryGestures[size_t(index)].exchange(true))historyGestures.fetch_add(1);}
    else{if(activeHistoryGestures[size_t(index)].exchange(false))historyGestures.fetch_sub(1);if(canCapture)captureHistory();}
}
void PhaseTwinProcessor::beginHistoryAction(){captureHistory();++historyActionDepth;}
void PhaseTwinProcessor::endHistoryAction(){jassert(historyActionDepth>0);--historyActionDepth;captureHistory();}
void PhaseTwinProcessor::restoreAudioSettings(const phasetwin::AudioSettings& state){
    historySuppressed=true;
    for(size_t i=0;i<historyParameters.size();++i)if(phasetwin::historyParameter(i) && historyParameters[i]->getValue()!=state.parameters[i]){
        auto* parameter=historyParameters[i];parameter->beginChangeGesture();parameter->setValueNotifyingHost(state.parameters[i]);parameter->endChangeGesture();
    }
    historySuppressed=false;hearProposalRequested=false;recommendationReady=false;
    std::uint32_t bits=0;std::memcpy(&bits,&state.lagMs,sizeof(bits));restoreCorrectionPacked.store(std::uint64_t(bits) | (std::uint64_t(state.polarity)<<32));restoreRequest.fetch_add(1,std::memory_order_release);trackingHistoryStep=false;
    updateHistoryAvailability();
}
void PhaseTwinProcessor::undoAudio(){captureHistory();if(historyGestures.load()>0 || resetRequest.load()!=resetAcknowledged.load())return;if(auto* state=history.undo())restoreAudioSettings(*state);}
void PhaseTwinProcessor::redoAudio(){captureHistory();if(historyGestures.load()>0 || resetRequest.load()!=resetAcknowledged.load())return;if(auto* state=history.redo())restoreAudioSettings(*state);}

void PhaseTwinProcessor::prepareToPlay(double sr,int){
    stopThread(-1);
    if(!std::isfinite(sr) || sr<8000)sr=48000;
    detectedLag.store(float(detectedLag.load()*sr/sampleRateHz));
    sampleRateHz=sr;
    const double lag=detectedLag.load()+sr*values[4]->load()/1000.0;
    const bool invert=inverted.load()!=(values[5]->load()>0.5f);
    rotator.prepare(sr);spectrumAvailable=false;
    ducker.prepare(sr);duckReductionDb=0;engine.prepare(sr,lag,invert);setLatencySamples(engine.getLatency());
    outputPeakL=outputPeakR=-100;outputClipped=false;seenClearClip=clearClipRequest.load();
    hearProposalRequested=false;hearingProposal=false;recommendationReady=false;seenApplyRequest=applyRequest.load();
    kickMailbox=0;kickStarted=false;checkMailbox=0;sectionMailbox=0;verifyActive=verifyStarted=false;verifyState=0;verifyProgress=0;clearSectionSession();workerSectionEpoch=sectionEpoch.load();sectionBank->clear();
    captureIndex=scopeIndex=0;scopeSampleClock=0;++scopeGeneration;mailbox=0;confidence=0;locked=false;referencePresent=false;
    verified=false;verificationAvailable=false;postCorrelation=0;residualLag=0;settling=false;
    inputDbA=-100;inputDbB=-100;bpmAvailable=false;hostBpm=120;analysisReason=int(phasetwin::AnalysisReason::waiting);
    appliedLag=float(engine.appliedLag());
    ++generation;previousLag=lag;previousInvert=invert;captureSettled=true;
    seenStateRevision=stateRevision.load();seenAlignRequest=alignRequest.load();seenResetRequest=resetRequest.load();seenCancelRequest=cancelRequest.load();resetAcknowledged=seenResetRequest;
    learning.reset();learnActive=learnTimedOut=false;learnElapsed=0;learnState=0;learnProgress=0;reliability=0;
    scopePacket.sampleRate=sr;
    referenceMix.reset(sr,0.02);referenceMix.setCurrentAndTargetValue(values[6]->load());
    outputGain.reset(sr,0.02);outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(values[7]->load()));
    compareMix.reset(sr,0.02);compareMix.setCurrentAndTargetValue(values[12]->load());
    startThread();
}
void PhaseTwinProcessor::releaseResources(){stopThread(-1);}
bool PhaseTwinProcessor::isBusesLayoutSupported(const BusesLayout& l)const{
    if(l.inputBuses.size()!=2 || l.outputBuses.size()!=1)return false;
    auto main=l.getMainInputChannelSet();auto side=l.getChannelSet(true,1);
    return (main==juce::AudioChannelSet::mono()||main==juce::AudioChannelSet::stereo()) && main==l.getMainOutputChannelSet() && (side.isDisabled()||side==juce::AudioChannelSet::mono()||side==juce::AudioChannelSet::stereo());
}
void PhaseTwinProcessor::clearSectionSession(){sectionEpoch.fetch_add(1);sectionCount=0;for(auto& gain:sectionGains)gain=0;}
void PhaseTwinProcessor::syncSectionBank(){const auto epoch=sectionEpoch.load();if(epoch!=workerSectionEpoch){sectionBank->clear();workerSectionEpoch=epoch;}}
void PhaseTwinProcessor::startVerification(){++verifyEpoch;verifyActive=values[28]->load()>.5f;verifyStarted=false;verifyWait=verifyElapsed=0;verifyAccumulator.reset();verifyState=verifyActive?1:0;verifyProgress=0;verifyCount=0;}
void PhaseTwinProcessor::publishVerification(const phasetwin::VerificationResult& result){verifyBefore=float(result.before);verifyAfter=float(result.after);verifyConfidence=float(result.confidence);verifyCount=result.count;verifyState=result.state;verifyProgress=1;verifyActive=verifyStarted=false;}
void PhaseTwinProcessor::run(){
    while(!threadShouldExit()){
        syncSectionBank();
        if(checkMailbox.load(std::memory_order_acquire)==1){checkResult=phasetwin::verifyKickPair(verifyKickBefore,verifyKickAfter);checkMailbox.store(2,std::memory_order_release);}
        if(sectionMailbox.load(std::memory_order_acquire)==1){
            syncSectionBank();
            if(sectionCommandEpoch==workerSectionEpoch){if(sectionCommand==1){sectionBank->finishSource();sectionResult={};sectionResult.count=sectionBank->count();}else sectionResult=sectionBank->analyse(sectionConfig);}
            else sectionResult={};
            sectionMailbox.store(2,std::memory_order_release);
        }
        if(kickMailbox.load(std::memory_order_acquire)==1){kickResult=kickAnalyzer.analyse(kickData,kickConfig);kickSectionCount=0;syncSectionBank();
            if(kickJobMulti && kickSectionEpoch==workerSectionEpoch){sectionBank->begin(true,phasetwin::kickRate);sectionBank->addKick(kickData,kickResult);kickSectionCount=sectionBank->count();}
            kickMailbox.store(2,std::memory_order_release);
        }
        if(mailbox.load(std::memory_order_acquire)!=1){wait(5);continue;}
        jobChannel=jobLowEnd?phasetwin::AnalysisBand::selectFilteredChannel(jobA[0].data(),jobA[1].data(),jobB[0].data(),jobB[1].data(),bandA,bandB,jobSampleRate,jobCutoff):phasetwin::strongestPair(jobA[0].data(),jobA[1].data(),jobB[0].data(),jobB[1].data(),phasetwin::frameSize);
        const float *a=jobLowEnd?bandA[jobChannel].data():jobA[jobChannel].data(),*b=jobLowEnd?bandB[jobChannel].data():jobB[jobChannel].data();
        const float *pa=jobPostA[jobChannel].data(),*pb=jobPostB[jobChannel].data();
        if(jobLowEnd){
            phasetwin::AnalysisBand::filter(pa,bandPostA.data(),phasetwin::frameSize,jobSampleRate,jobCutoff);
            phasetwin::AnalysisBand::filter(pb,bandPostB.data(),phasetwin::frameSize,jobSampleRate,jobCutoff);
            pa=bandPostA.data();pb=bandPostB.data();
        }
        const int exclusion=jobLowEnd?int(jobSampleRate/(3*jobCutoff)):8;
        jobEstimate=estimator.analyse(a,b,jobSearch,jobGate,exclusion);
        jobVerification=estimator.analyse(pa,pb,jobSearch,jobGate,exclusion);
        jobSpectrum=spectralAnalyzer.analyse(jobA[jobChannel].data(),jobB[jobChannel].data(),jobPostA[jobChannel].data(),jobPostB[jobChannel].data(),jobSampleRate);
        jobBeforeCorrelation=phasetwin::zeroLagCorrelation(a,b,phasetwin::frameSize);
        jobPostCorrelation=phasetwin::zeroLagCorrelation(pa,pb,phasetwin::frameSize);
        jobNeutralCorrelation=phasetwin::zeroLagCorrelation(jobNeutralA[jobChannel].data(),jobPostB[jobChannel].data(),phasetwin::frameSize);
        syncSectionBank();
        if(jobSectionCollect && jobSectionEpoch==workerSectionEpoch){sectionBank->begin(false,jobSampleRate);sectionBank->addSourceFrame(jobA,jobB,jobEstimate);}
        // Publish the lag and polarity as one coherent result. Only the audio
        // thread commits corrections; stale jobs cannot override a new state.
        mailbox.store(2,std::memory_order_release);
    }
}
void PhaseTwinProcessor::propose(double lag,bool polarity,double improvement){
    hearProposalRequested=false;
    proposedLagMs=float(lag*1000/sampleRateHz);proposedPolarity=polarity;proposedGain=float(improvement);recommendationGeneration=generation;recommendationReady=true;learnState=7;
}
void PhaseTwinProcessor::commitLearned(double lag,bool polarity){
    if(std::abs(lag-detectedLag.load())<1e-5 && polarity==inverted.load())return;
    detectedLag=float(lag);inverted=polarity;publishHeldCorrection();startVerification();clearSectionSession();
}
void PhaseTwinProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){process(b,false);}
void PhaseTwinProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&){process(b,true);}
void PhaseTwinProcessor::process(juce::AudioBuffer<float>& buffer,bool bypass){
    juce::ScopedNoDenormals noDenormals;
    audioBlocks.fetch_add(1,std::memory_order_relaxed);
    bool gotTempo=bpmAvailable.load();
    if(auto* playhead=getPlayHead())if(auto position=playhead->getPosition())if(auto bpm=position->getBpm()){
        if(std::isfinite(*bpm) && *bpm>0){hostBpm.store(*bpm);gotTempo=true;}
    }
    bpmAvailable.store(gotTempo);
    auto main=getBusBuffer(buffer,true,0);auto output=getBusBuffer(buffer,false,0);
    const bool hasReference=getBus(true,1)->isEnabled();
    auto side=getBusBuffer(buffer,true,1);referencePresent.store(hasReference);
    const bool automatic=values[0]->load()>0.5f,freeze=values[1]->load()>0.5f;
    const int mode=int(values[8]->load());const float band=values[11]->load();
    const int correctionMode=std::clamp(int(values[16]->load()),0,2);
    const bool allowTiming=correctionMode!=2,allowPolarity=correctionMode!=1;
    const float trim=values[4]->load(),polarity=values[5]->load(),gate=values[3]->load();
    const int search=std::clamp(int(sampleRateHz*values[2]->load()/1000),1,phasetwin::frameSize/2-1);
    const bool groove=values[17]->load()>.5f,preview=values[19]->load()>.5f;const float grooveLimit=values[18]->load();
    const bool rotation=values[25]->load()>.5f;const float rotationFrequency=values[26]->load(),rotationQ=values[27]->load();
    const bool multi=values[29]->load()>.5f,checkEnabled=values[28]->load()>.5f;
    if(multi!=previousMulti){previousMulti=multi;clearSectionSession();recommendationReady=false;learnActive=false;kickStarted=false;learning.reset();captureIndex=0;++generation;learnState=0;}
    if(checkEnabled!=previousVerify){previousVerify=checkEnabled;if(!checkEnabled){verifyActive=verifyStarted=false;++verifyEpoch;verifyState=0;}}
    if(clearSectionsRequest.load()!=seenClearSections){seenClearSections=clearSectionsRequest.load();clearSectionSession();recommendationReady=false;++generation;learnActive=false;kickStarted=false;learning.reset();learnState=0;}
    const auto revision=stateRevision.load();
    const auto request=alignRequest.load();
    if(request!=seenAlignRequest && (!multi || sectionCount.load()<phasetwin::maxSections)){
        recommendationReady=false;seenAlignRequest=request;++generation;captureIndex=0;captureSettled=true;verifyActive=verifyStarted=false;++verifyEpoch;verifyState=0;
        kickStarted=false;learning.reset();learnActive=true;learnTimedOut=false;learnElapsed=0;learnState=1;learnProgress=0;reliability=0;
        analysisReason=int(phasetwin::AnalysisReason::waiting);verified=false;verificationAvailable=false;
    }
    if(cancelRequest.load()!=seenCancelRequest){
        seenCancelRequest=cancelRequest.load();clearSectionSession();recommendationReady=false;++generation;captureIndex=0;captureSettled=true;kickStarted=false;learning.reset();learnActive=false;learnProgress=0;learnState=6;
        verified=false;verificationAvailable=false;reliability=0;
    }
    if(restoreRequest.load(std::memory_order_acquire)!=seenRestoreRequest){
        seenRestoreRequest=restoreRequest.load();const auto packed=restoreCorrectionPacked.load();const auto bits=std::uint32_t(packed);float ms=0;std::memcpy(&ms,&bits,sizeof(bits));detectedLag=ms*float(sampleRateHz/1000);inverted=(packed>>32)!=0;publishHeldCorrection();
        recommendationReady=false;hearProposalRequested=false;clearSectionSession();verifyActive=verifyStarted=false;++verifyEpoch;verifyState=0;++generation;captureIndex=0;captureSettled=true;kickStarted=false;learning.reset();learnActive=false;learnProgress=0;learnState=5;
        verified=false;verificationAvailable=false;reliability=0;restoreAcknowledged.store(seenRestoreRequest,std::memory_order_release);
    }
    if(resetRequest.load()!=seenResetRequest){
        seenResetRequest=resetRequest.load();
        if(seenResetRequest!=ignoreResetRequest.load()){
            recommendationReady=false;clearSectionSession();verifyActive=verifyStarted=false;++verifyEpoch;verifyState=0;detectedLag=0;inverted=false;publishHeldCorrection();learning.reset();learnActive=false;learnState=0;learnProgress=0;reliability=0;
            ++generation;captureIndex=0;captureSettled=true;
        }
        resetAcknowledged.store(seenResetRequest,std::memory_order_release);
    }
    if(hasReference!=previousReference)++scopeGeneration;
    if(hasReference!=previousReference || bypass!=previousBypass || automatic!=previousAuto || freeze!=previousFreeze || trim!=previousTrim || polarity!=previousPolarity || search!=previousSearch || gate!=previousGate || revision!=seenStateRevision || mode!=previousMode || band!=previousBand || allowTiming!=previousAllowTiming || allowPolarity!=previousAllowPolarity || groove!=previousGroove || grooveLimit!=previousGrooveLimit || preview!=previousPreview || rotation!=previousRotation || rotationFrequency!=previousRotationFrequency || rotationQ!=previousRotationQ){
        clearSectionSession();if(learnState.load()==9 || learnState.load()==10)learnState=0;verifyActive=verifyStarted=false;++verifyEpoch;verifyState=0;
        if(recommendationReady.exchange(false) && !learnActive)learnState=0;
        ++generation;captureIndex=0;captureSettled=true;
        spectrumAvailable=false;verified=false;verificationAvailable=false;postCorrelation=0;
        previousReference=hasReference;previousBypass=bypass;previousAuto=automatic;previousFreeze=freeze;
        previousTrim=trim;previousPolarity=polarity;previousSearch=search;previousGate=gate;seenStateRevision=revision;previousMode=mode;previousBand=band;previousAllowTiming=allowTiming;previousAllowPolarity=allowPolarity;previousGroove=groove;previousGrooveLimit=grooveLimit;previousPreview=preview;previousRotation=rotation;previousRotationFrequency=rotationFrequency;previousRotationQ=rotationQ;
        kickStarted=false;learning.reset();reliability=0;if(learnActive){learnElapsed=0;learnTimedOut=false;learnProgress=0;}
    }
    if(applyRequest.load()!=seenApplyRequest){seenApplyRequest=applyRequest.load();if(recommendationReady.exchange(false) && recommendationGeneration==generation && !bypass && hasReference && !freeze){commitLearned(proposedLagMs.load()*sampleRateHz/1000,proposedPolarity.load());++generation;learning.reset();captureIndex=0;captureSettled=true;learnActive=false;kickStarted=false;learnState=2;}else if(learnState.load()==7)learnState=6; }
    if(mailbox.load(std::memory_order_acquire)==2){
        if(jobGeneration==generation && hasReference && !bypass){
            spectralEpoch.fetch_add(1,std::memory_order_acq_rel);
            for(int band=0;band<phasetwin::spectralBands;++band){spectralBefore[band]=jobSpectrum.before[band];spectralAfter[band]=jobSpectrum.after[band];spectralConfidence[band]=jobSpectrum.confidence[band];spectralLevelDb[band]=jobSpectrum.levelDb[band];}spectralSampleRate=jobSpectrum.sampleRate;spectrumAvailable=true;spectralEpoch.fetch_add(1,std::memory_order_release);
            confidence=float(jobEstimate.confidence);locked=jobEstimate.valid;
            analysisReason=int(jobEstimate.reason);analysisChannel=jobChannel;
            if(mode==0){beforeScore=phasetwin::coherenceScore(jobBeforeCorrelation);alignmentScore=phasetwin::coherenceScore(jobPostCorrelation);}
            if(mode==0 && !multi && (learnActive || automatic)){
                learning.add(jobEstimate);
                const auto candidate=learning.result(sampleRateHz,mode==1);reliability=float(candidate.confidence);
                if(automatic && !learnActive && !freeze && candidate.reliable && !recommendationReady.load()){
                    if(preview){const double nextLag=allowTiming?candidate.lag:detectedLag.load();const bool nextPolarity=allowPolarity?candidate.inverted:inverted.load();
                        const double gain=phasetwin::predictedCorrelation(jobA[jobChannel].data(),jobB[jobChannel].data(),phasetwin::frameSize,nextLag+trim*sampleRateHz/1000,nextPolarity!=(polarity>.5f))-phasetwin::predictedCorrelation(jobA[jobChannel].data(),jobB[jobChannel].data(),phasetwin::frameSize,detectedLag.load()+trim*sampleRateHz/1000,inverted.load()!=(polarity>.5f));
                        if(gain>.01)propose(nextLag,nextPolarity,gain);
                    }
                    else {
                    if(allowTiming && std::abs(candidate.lag-detectedLag.load())>0.05)detectedLag=float(candidate.lag);
                    if(allowPolarity)inverted=candidate.inverted;
                    publishHeldCorrection();}
                }
            }
            if(verifyActive && mode==0 && verifyStarted && jobVerifyEpoch==verifyEpoch && jobWasSettled){verifyAccumulator.add(jobNeutralCorrelation,jobPostCorrelation,jobEstimate.valid);verifyCount=verifyAccumulator.result().count;if(verifyCount.load()>=5 && verifyElapsed>=std::uint64_t(sampleRateHz))publishVerification(verifyAccumulator.result(5));}
            postCorrelation=float(jobPostCorrelation);
            residualLag=float(jobVerification.lag);
            verificationAvailable=mode==0 && jobVerification.valid && jobWasSettled;
            verified=mode==0 && jobWasSettled && jobVerification.valid && !jobVerification.inverted && std::abs(jobVerification.lag)<0.5 && jobPostCorrelation>=gate;
        }
        mailbox.store(0,std::memory_order_release);
    }
    if(kickMailbox.load(std::memory_order_acquire)==2){
        if(kickGeneration==generation && mode==1 && learnActive && hasReference && !bypass && !freeze){
            beforeScore=phasetwin::kickScore(kickResult.before);alignmentScore=phasetwin::kickScore(kickResult.after);reliability=float(kickResult.confidence);
            if(kickJobMulti && kickSectionEpoch==sectionEpoch.load()){sectionCount=kickSectionCount;learnState=9;}
            else if(!kickConfig.allowTiming && !kickConfig.allowPolarity)learnState=4;
            else if(kickResult.reliable && kickResult.improvement>.02){const double nextLag=kickConfig.allowTiming?kickResult.lag*sampleRateHz/phasetwin::kickRate:detectedLag.load();const bool nextPolarity=kickConfig.allowPolarity?kickResult.inverted:inverted.load();if(preview)propose(nextLag,nextPolarity,kickResult.improvement);else{commitLearned(nextLag,nextPolarity);learnState=2;}}else{learnState=8;recommendationReason=kickResult.reason;}
            learnActive=false;learnProgress=1;
        }
        kickMailbox.store(0,std::memory_order_release);if(learnActive)kickStarted=false;
    }
    if(learnActive && (freeze || bypass || !hasReference)){learnActive=false;learnState=3;}
    if(learnActive && mode==1 && !kickStarted && kickMailbox.load(std::memory_order_acquire)==0){
        kickData.count=0;kickCapture.prepare(sampleRateHz,band);kickStarted=true;
        kickConfig={std::clamp(int(phasetwin::kickRate*values[2]->load()/1000),1,120),detectedLag.load()*phasetwin::kickRate/sampleRateHz,trim*phasetwin::kickRate/1000.0,inverted.load(),polarity>0.5f,allowTiming,allowPolarity,groove,grooveLimit*phasetwin::kickRate/1000};
        kickGeneration=generation;kickJobMulti=multi;kickSectionEpoch=sectionEpoch.load();
    }
    if(mode==0 && multi && learnActive && learnTimedOut && mailbox.load(std::memory_order_acquire)==0 && sectionMailbox.load(std::memory_order_acquire)==0){sectionCommand=1;sectionCommandEpoch=sectionEpoch.load();sectionCommandGeneration=generation;sectionMailbox.store(1,std::memory_order_release);learnActive=false;learnState=10;}
    if(mode==0 && !multi && learnActive && learnTimedOut && mailbox.load(std::memory_order_acquire)!=1){
        const auto candidate=learning.result(sampleRateHz,mode==1);reliability=float(candidate.confidence);
        if(!allowTiming && !allowPolarity)learnState=4;
        else if(candidate.reliable && !freeze){
            const double nextLag=allowTiming?candidate.lag:detectedLag.load();const bool nextPolarity=allowPolarity?candidate.inverted:inverted.load();
            const double baseline=phasetwin::predictedCorrelation(jobA[jobChannel].data(),jobB[jobChannel].data(),phasetwin::frameSize,detectedLag.load()+trim*sampleRateHz/1000,inverted.load()!=(polarity>.5f));
            const double predicted=phasetwin::predictedCorrelation(jobA[jobChannel].data(),jobB[jobChannel].data(),phasetwin::frameSize,nextLag+trim*sampleRateHz/1000,nextPolarity!=(polarity>.5f));
            if(predicted-baseline<.01){learnState=8;recommendationReason=3;}
            else if(preview)propose(nextLag,nextPolarity,predicted-baseline);
            else{commitLearned(nextLag,nextPolarity);learnState=2;}
        }else{learnState=8;recommendationReason=5;}
        learnActive=false;
    }
    if(sectionMailbox.load(std::memory_order_acquire)==2){
        if(sectionCommandEpoch==sectionEpoch.load() && sectionCommandGeneration==generation){
            sectionCount=sectionResult.count;
            if(sectionCommand==1)learnState=9;
            else if(sectionResult.reliable){reliability=float(sectionResult.confidence);beforeScore=mode==1?phasetwin::kickScore(sectionResult.before):phasetwin::coherenceScore(sectionResult.before);alignmentScore=mode==1?phasetwin::kickScore(sectionResult.after):phasetwin::coherenceScore(sectionResult.after);for(int s=0;s<phasetwin::maxSections;++s)sectionGains[s]=float(sectionResult.gains[s]);const double nextLag=sectionResult.lag*(mode==1?sampleRateHz/phasetwin::kickRate:1);if(preview)propose(nextLag,sectionResult.inverted,sectionResult.improvement);else{commitLearned(nextLag,sectionResult.inverted);learnState=2;}}
            else{learnState=8;recommendationReason=sectionResult.reason==1?6:sectionResult.reason==2?7:8;}
        }
        if(sectionCommandGeneration!=generation && learnState.load()==10)learnState=0;
        sectionMailbox.store(0,std::memory_order_release);
    }
    if(evaluateSectionsRequest.load()!=seenEvaluateSections && sectionMailbox.load(std::memory_order_acquire)==0){
        seenEvaluateSections=evaluateSectionsRequest.load();
        if(multi && sectionCount.load()>=2 && !learnActive && !bypass && hasReference && !freeze){
            const double analysisRate=mode==1?phasetwin::kickRate:sampleRateHz;sectionConfig={std::clamp(int(analysisRate*values[2]->load()/1000),1,phasetwin::frameSize/2-1),detectedLag.load()*analysisRate/sampleRateHz,trim*analysisRate/1000,inverted.load(),polarity>.5f,allowTiming,allowPolarity,groove,grooveLimit*analysisRate/1000};
            sectionCommand=2;sectionCommandEpoch=sectionEpoch.load();sectionCommandGeneration=generation;sectionMailbox.store(1,std::memory_order_release);learnState=10;
        }
    }
    if(checkMailbox.load(std::memory_order_acquire)==2){if(checkEpoch==verifyEpoch && verifyActive)publishVerification(checkResult);checkMailbox.store(0,std::memory_order_release);}
    if(!recommendationReady.load() || bypass || !hasReference || freeze)hearProposalRequested=false;
    const bool hearProposal=hearProposalRequested.load() && recommendationReady.load();hearingProposal=hearProposal;
    const int storedAudition=std::clamp(int(values[20]->load()),0,3),audition=hearProposal?0:storedAudition;
    const double heardLag=hearProposal?proposedLagMs.load()*sampleRateHz/1000:double(detectedLag.load());
    double lag=heardLag+sampleRateHz*trim/1000.0;
    lag=(bypass || audition==2 || audition==3)?0.0:std::clamp(lag,-double(engine.getMaximumLag()),double(engine.getMaximumLag()));
    const bool heardPolarity=hearProposal?proposedPolarity.load():inverted.load();
    const bool invert=!bypass && audition!=1 && audition!=3 && (heardPolarity!=(polarity>0.5f));
    if(std::abs(lag-previousLag)>0.001 || invert!=previousInvert){
        ++generation;if(recommendationReady.load())recommendationGeneration=generation;captureIndex=0;captureSettled=true;kickStarted=false;
        verified=false;verificationAvailable=false;previousLag=lag;previousInvert=invert;
    }
    engine.setCorrection(lag,invert);
    rotator.configure(rotation && !bypass && audition!=1 && audition!=3 && (hearProposal || values[12]->load()<.5f),rotationFrequency,rotationQ);
    referenceMix.setTargetValue(bypass?0.0f:values[6]->load());outputGain.setTargetValue(bypass?1.0f:juce::Decibels::decibelsToGain(values[7]->load()));
    compareMix.setTargetValue(!hearProposal && values[12]->load()>0.5f?1.0f:0.0f);
    ducker.configure(!bypass && hasReference && values[13]->load()>0.5f,values[14]->load(),values[15]->load(),values[21]->load()>.5f,values[22]->load(),values[23]->load(),values[24]->load());
    scopePacket.sampleRate=sampleRateHz;scopePacket.generation=scopeGeneration;
    if(clearClipRequest.load()!=seenClearClip){seenClearClip=clearClipRequest.load();outputClipped=false;}
    const bool checkAudition=!bypass && hasReference && !hearProposal && audition==0 && values[12]->load()<.5f;
    if(verifyActive && (!checkAudition || engine.isSettling() || rotator.isSettling() || compareMix.isSmoothing())){verifyStarted=false;verifyWait=verifyElapsed=0;verifyAccumulator.reset();verifyState=1;verifyProgress=0;++verifyEpoch;}
    if(verifyActive && !verifyStarted && checkAudition && !engine.isSettling() && !rotator.isSettling() && !compareMix.isSmoothing() && checkMailbox.load(std::memory_order_acquire)==0){
        verifyWait+=std::uint64_t(buffer.getNumSamples());if(verifyWait>=std::uint64_t(sampleRateHz*.06)){verifyStarted=true;verifyElapsed=0;verifyState=2;captureIndex=0;captureSettled=true;verifyKickBefore.count=verifyKickAfter.count=0;if(mode==1){verifyCaptureBefore.prepare(sampleRateHz,band);verifyCaptureAfter.prepare(sampleRateHz,band);}}
    }
    std::array<float,2> outputPeaks{};
    double inputEnergyA=0,inputEnergyB=0;
    for(int n=0;n<buffer.getNumSamples();++n){
        std::array<float,2> a{},b{};
        // Read all source channels before writing the overlapping output bus.
        for(int ch=0;ch<main.getNumChannels();++ch){float x=main.getSample(ch,n);a[ch]=std::isfinite(x)?x:0.0f;}
        if(main.getNumChannels()==1)a[1]=a[0];
        if(hasReference)for(int ch=0;ch<side.getNumChannels();++ch){float x=side.getSample(ch,n);b[ch]=std::isfinite(x)?x:0.0f;}
        if(hasReference && side.getNumChannels()==1)b[1]=b[0];
        auto mappedB=b;
        if(main.getNumChannels()==1)mappedB[0]=mappedB[1]=0.5f*(b[0]+b[1]);
        auto corrected=engine.process(a,mappedB);
        const float neutral=compareMix.getNextValue();
        for(int ch=0;ch<2;++ch)corrected.a[ch]=corrected.a[ch]*(1-neutral)+corrected.neutralA[ch]*neutral;
        corrected.a=rotator.process(corrected.a);
        if(verifyActive && verifyStarted){
            ++verifyElapsed;verifyProgress=float(std::min(1.0,double(verifyElapsed)/(sampleRateHz*(mode==1?4:2))));
            if(mode==1 && checkMailbox.load(std::memory_order_relaxed)==0){const bool done=verifyCaptureBefore.push(verifyKickBefore,corrected.neutralA,corrected.b);verifyCaptureAfter.push(verifyKickAfter,corrected.a,corrected.b);if(done){checkEpoch=verifyEpoch;checkMailbox.store(1,std::memory_order_release);verifyStarted=false;}}
            else if(mode==0 && verifyElapsed>=std::uint64_t(sampleRateHz*2)){publishVerification(verifyAccumulator.result(5));}
        }
        if(learnActive && mode==1 && kickStarted && kickMailbox.load(std::memory_order_relaxed)==0){
            if(kickCapture.push(kickData,a,mappedB))kickMailbox.store(1,std::memory_order_release);
            learnProgress=float(kickData.count)/phasetwin::kickCaptureSize;
        }
        if(learnActive && mode==0 && !learnTimedOut){++learnElapsed;learnProgress=float(std::min(1.0,learnElapsed/(2*sampleRateHz)));if(learnElapsed>=std::uint64_t(2*sampleRateHz))learnTimedOut=true;}
        inputEnergyA+=0.5*(double(a[0])*a[0]+double(a[1])*a[1]);
        inputEnergyB+=0.5*(double(b[0])*b[0]+double(b[1])*b[1]);
        if(hasReference && !bypass){
            for(int ch=0;ch<2;++ch){captureA[ch][captureIndex]=a[ch];captureB[ch][captureIndex]=mappedB[ch];
                capturePostA[ch][captureIndex]=corrected.a[ch];capturePostB[ch][captureIndex]=corrected.b[ch];captureNeutralA[ch][captureIndex]=corrected.neutralA[ch];}
            captureSettled=captureSettled && !engine.isSettling() && !compareMix.isSmoothing() && !rotator.isSettling();
            if(++captureIndex==phasetwin::frameSize){
                captureIndex=0;
                if(mailbox.load(std::memory_order_acquire)==0){
                    jobA=captureA;jobB=captureB;jobPostA=capturePostA;jobPostB=capturePostB;jobNeutralA=captureNeutralA;jobSectionCollect=multi && learnActive && mode==0;jobSectionEpoch=sectionEpoch.load();jobVerifyEpoch=verifyStarted?verifyEpoch:0;
                    jobSearch=search;jobGate=gate;jobGeneration=generation;jobWasSettled=captureSettled;jobLowEnd=mode==1;jobSampleRate=sampleRateHz;jobCutoff=band;
                    mailbox.store(1,std::memory_order_release);
                }
                captureSettled=true;
            }
        }else captureIndex=0;
        const float duckGain=ducker.process(corrected.b);
        if(scopeIndex==0)scopePacket.firstSample=scopeSampleClock;
        ++scopeSampleClock;
        for(int ch=0;ch<2;++ch){
            scopePacket.traces[ch][scopeIndex]=a[ch];scopePacket.traces[2+ch][scopeIndex]=mappedB[ch];
            scopePacket.traces[4+ch][scopeIndex]=corrected.a[ch];scopePacket.traces[6+ch][scopeIndex]=corrected.b[ch];
            scopePacket.traces[8+ch][scopeIndex]=duckGain*corrected.a[ch];
        }
        scopePacket.reductionDb[scopeIndex]=ducker.reductionDb();
        if(++scopeIndex==phasetwin::scopePacketSamples){scopeIndex=0;scope.push(scopePacket);}
        const float mix=referenceMix.getNextValue(),gain=outputGain.getNextValue();
        for(int ch=0;ch<output.getNumChannels();++ch){const float value=gain*(duckGain*corrected.a[ch]+mix*corrected.b[ch]);output.setSample(ch,n,value);outputPeaks[ch]=std::max(outputPeaks[ch],std::abs(value));if(std::abs(value)>=1)outputClipped=true;}
    }
    if(buffer.getNumSamples()>0){
        if(output.getNumChannels()==1)outputPeaks[1]=outputPeaks[0];
        const float decay=float(24.0*buffer.getNumSamples()/sampleRateHz);
        outputPeakL=std::max(outputPeakL.load()-decay,float(20*std::log10(std::max(1e-5f,outputPeaks[0]))));outputPeakR=std::max(outputPeakR.load()-decay,float(20*std::log10(std::max(1e-5f,outputPeaks[1]))));
        const float release=float(60.0*buffer.getNumSamples()/sampleRateHz);
        inputDbA=std::max(inputDbA.load()-release,float(10*std::log10(std::max(1e-10,inputEnergyA/buffer.getNumSamples()))));
        inputDbB=std::max(inputDbB.load()-release,float(10*std::log10(std::max(1e-10,inputEnergyB/buffer.getNumSamples()))));
    }
    duckReductionDb=ducker.reductionDb();
    appliedLag=float(engine.appliedLag());settling=engine.isSettling();
    if(engine.isSettling())verified=false;
    if(!hasReference || bypass){spectrumAvailable=false;confidence=0;locked=false;verified=false;verificationAvailable=false;postCorrelation=0;alignmentScore=0;}
}
void PhaseTwinProcessor::getStateInformation(juce::MemoryBlock& block){auto state=parameters.copyState();state.setProperty("schema",12,nullptr);for(auto* old:{"undoAvailable","priorLagMs","priorPolarity"})state.removeProperty(old,nullptr);state.setProperty("editorSections",int(editorSections.load() & 31u),nullptr);state.setProperty("scopeView",int(scopePreferences.load()),nullptr);const auto held=readAudioSettings();state.setProperty("heldLagMs",held.lagMs,nullptr);state.setProperty("heldPolarity",held.polarity,nullptr);if(auto xml=state.createXml())copyXmlToBinary(*xml,block);}
void PhaseTwinProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType())){historySuppressed=true;auto state=juce::ValueTree::fromXml(*xml);editorSections=unsigned(int(state.getProperty("editorSections",0))) & 31u;scopePreferences=std::uint32_t(int(state.getProperty("scopeView",int(phasetwin::ScopePreferences{}.packed()))));scopeViewRevision.fetch_add(1);const double heldMs=double(state.getProperty("heldLagMs",0.0));detectedLag.store(float((std::isfinite(heldMs)?std::clamp(heldMs,-40.0,40.0):0.0)*sampleRateHz/1000.0));inverted.store(bool(state.getProperty("heldPolarity",false)));if(int(state.getProperty("schema",0))<3){auto oldAuto=state.getChildWithProperty("id","auto");if(oldAuto.isValid() && double(oldAuto.getProperty("value",1.0))<0.5){detectedLag=0;inverted=false;}}
        const char* duckIds[]={"duckEnabled","duckAmount","duckHarshness"};
        for(int i=0;i<3;++i)if(!state.getChildWithProperty("id",duckIds[i]).isValid()){juce::ValueTree parameter("PARAM");parameter.setProperty("id",duckIds[i],nullptr);parameter.setProperty("value",i==0?0:50,nullptr);state.appendChild(parameter,nullptr);}
        if(!state.getChildWithProperty("id","correctionMode").isValid()){
            const auto timing=state.getChildWithProperty("id","allowDelay"),polarity=state.getChildWithProperty("id","allowPolarity");
            const bool timingAllowed=!timing.isValid() || double(timing.getProperty("value",1))>=0.5;
            const bool polarityAllowed=!polarity.isValid() || double(polarity.getProperty("value",1))>=0.5;
            juce::ValueTree correction("PARAM");correction.setProperty("id","correctionMode",nullptr);correction.setProperty("value",!timingAllowed?2:!polarityAllowed?1:0,nullptr);state.appendChild(correction,nullptr);
            // Legacy measure-only sessions load locked rather than suddenly adapting.
            if(!timingAllowed && !polarityAllowed){auto lock=state.getChildWithProperty("id","freeze");if(!lock.isValid()){lock=juce::ValueTree("PARAM");lock.setProperty("id","freeze",nullptr);state.appendChild(lock,nullptr);}lock.setProperty("value",1,nullptr);}
        }
        const char* newIds[]={"preserveGroove","grooveLimit","preview","audition","duckAdvanced","duckAttack","duckRelease","duckSensitivity","rotation","rotationFrequency","rotationQ"};const float defaults[]={1,2,1,0,0,3,125,0,0,80,.707f};
        for(int i=0;i<11;++i)if(!state.getChildWithProperty("id",newIds[i]).isValid()){juce::ValueTree parameter("PARAM");parameter.setProperty("id",newIds[i],nullptr);parameter.setProperty("value",defaults[i],nullptr);state.appendChild(parameter,nullptr);}
        const char* sessionIds[]={"verifyAfterApply","multiSection"};for(int i=0;i<2;++i)if(!state.getChildWithProperty("id",sessionIds[i]).isValid()){juce::ValueTree parameter("PARAM");parameter.setProperty("id",sessionIds[i],nullptr);parameter.setProperty("value",i==0?1:0,nullptr);state.appendChild(parameter,nullptr);}
        parameters.replaceState(state);recommendationReady=false;hearProposalRequested=false;cancelRequest.fetch_add(1);ignoreResetRequest=resetRequest.load();publishHeldCorrection();restoreCorrectionPacked=heldCorrectionPacked.load();restoreRequest.fetch_add(1,std::memory_order_release);historyBaselineNeeded=true;undoAvailable=false;redoAvailable=false;undoSteps=redoSteps=0;historySuppressed=false;stateRevision.fetch_add(1);}}
juce::AudioProcessorEditor* PhaseTwinProcessor::createEditor(){return new PhaseTwinEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PhaseTwinProcessor();}
