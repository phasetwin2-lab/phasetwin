#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"allowDelay",1},"Learn may change timing",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"allowPolarity",1},"Learn may change polarity",true));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"band",1},"Low-end focus (Hz)",juce::NormalisableRange<float>(50,500,1),180));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"compare",1},"Compare neutral",false));
    return p;
}
PhaseTwinProcessor::PhaseTwinProcessor():AudioProcessor(BusesProperties().withInput("Target A",juce::AudioChannelSet::stereo(),true).withInput("Reference B",juce::AudioChannelSet::stereo(),true).withOutput("Aligned output",juce::AudioChannelSet::stereo(),true)),Thread("PhaseTwin analysis"),parameters(*this,nullptr,"PhaseTwin",layout()){
    const char* ids[]={"auto","freeze","range","confidence","manual","polarity","mix","gain","profile","allowDelay","allowPolarity","band","compare"};
    for(size_t i=0;i<values.size();++i)values[i]=parameters.getRawParameterValue(ids[i]);
}
PhaseTwinProcessor::~PhaseTwinProcessor(){stopThread(-1);}
void PhaseTwinProcessor::prepareToPlay(double sr,int){
    stopThread(-1);
    if(!std::isfinite(sr) || sr<8000)sr=48000;
    detectedLag.store(float(detectedLag.load()*sr/sampleRateHz));
    sampleRateHz=sr;
    const double lag=detectedLag.load()+sr*values[4]->load()/1000.0;
    const bool invert=inverted.load()!=(values[5]->load()>0.5f);
    engine.prepare(sr,lag,invert);setLatencySamples(engine.getLatency());
    kickMailbox=0;kickStarted=false;
    captureIndex=scopeIndex=0;scopeSampleClock=0;++scopeGeneration;mailbox=0;confidence=0;locked=false;referencePresent=false;
    verified=false;verificationAvailable=false;postCorrelation=0;residualLag=0;settling=false;
    inputDbA=-100;inputDbB=-100;bpmAvailable=false;hostBpm=120;analysisReason=int(phasetwin::AnalysisReason::waiting);
    appliedLag=float(engine.appliedLag());
    ++generation;previousLag=lag;previousInvert=invert;captureSettled=true;
    seenStateRevision=stateRevision.load();seenAlignRequest=alignRequest.load();seenResetRequest=resetRequest.load();seenCancelRequest=cancelRequest.load();seenUndoRequest=undoRequest.load();
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
void PhaseTwinProcessor::run(){
    while(!threadShouldExit()){
        if(kickMailbox.load(std::memory_order_acquire)==1){kickResult=kickAnalyzer.analyse(kickData,kickConfig);kickMailbox.store(2,std::memory_order_release);}
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
        jobBeforeCorrelation=phasetwin::zeroLagCorrelation(a,b,phasetwin::frameSize);
        jobPostCorrelation=phasetwin::zeroLagCorrelation(pa,pb,phasetwin::frameSize);
        // Publish the lag and polarity as one coherent result. Only the audio
        // thread commits corrections; stale jobs cannot override a new state.
        mailbox.store(2,std::memory_order_release);
    }
}
void PhaseTwinProcessor::commitLearned(double lag,bool polarity){
    if(std::abs(lag-detectedLag.load())<1e-5 && polarity==inverted.load())return;
    priorLagMs=float(1000*detectedLag.load()/sampleRateHz);priorPolarity=inverted.load();undoAvailable=true;
    detectedLag=float(lag);inverted=polarity;
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
    const bool allowTiming=values[9]->load()>0.5f,allowPolarity=values[10]->load()>0.5f;
    const float trim=values[4]->load(),polarity=values[5]->load(),gate=values[3]->load();
    const int search=std::clamp(int(sampleRateHz*values[2]->load()/1000),1,phasetwin::frameSize/2-1);
    const auto revision=stateRevision.load();
    const auto request=alignRequest.load();
    if(request!=seenAlignRequest){
        seenAlignRequest=request;++generation;captureIndex=0;captureSettled=true;
        kickStarted=false;learning.reset();learnActive=true;learnTimedOut=false;learnElapsed=0;learnState=1;learnProgress=0;reliability=0;
        analysisReason=int(phasetwin::AnalysisReason::waiting);verified=false;verificationAvailable=false;
    }
    if(cancelRequest.load()!=seenCancelRequest || undoRequest.load()!=seenUndoRequest){
        const bool undo=undoRequest.load()!=seenUndoRequest;
        seenCancelRequest=cancelRequest.load();seenUndoRequest=undoRequest.load();
        ++generation;captureIndex=0;captureSettled=true;kickStarted=false;learning.reset();learnActive=false;learnProgress=0;
        if(undo && undoAvailable.exchange(false)){detectedLag=priorLagMs.load()*float(sampleRateHz/1000);inverted=priorPolarity.load();learnState=5;}else learnState=6;
        verified=false;verificationAvailable=false;reliability=0;
    }
    if(resetRequest.load()!=seenResetRequest){
        seenResetRequest=resetRequest.load();undoAvailable=false;detectedLag=0;inverted=false;learning.reset();learnActive=false;learnState=0;learnProgress=0;reliability=0;
        ++generation;captureIndex=0;captureSettled=true;
    }
    if(hasReference!=previousReference)++scopeGeneration;
    if(hasReference!=previousReference || bypass!=previousBypass || automatic!=previousAuto || freeze!=previousFreeze || trim!=previousTrim || polarity!=previousPolarity || search!=previousSearch || gate!=previousGate || revision!=seenStateRevision || mode!=previousMode || band!=previousBand || allowTiming!=previousAllowTiming || allowPolarity!=previousAllowPolarity){
        ++generation;captureIndex=0;captureSettled=true;
        verified=false;verificationAvailable=false;postCorrelation=0;
        previousReference=hasReference;previousBypass=bypass;previousAuto=automatic;previousFreeze=freeze;
        previousTrim=trim;previousPolarity=polarity;previousSearch=search;previousGate=gate;seenStateRevision=revision;previousMode=mode;previousBand=band;previousAllowTiming=allowTiming;previousAllowPolarity=allowPolarity;
        kickStarted=false;learning.reset();reliability=0;if(learnActive){learnElapsed=0;learnTimedOut=false;learnProgress=0;}
    }
    if(mailbox.load(std::memory_order_acquire)==2){
        if(jobGeneration==generation && hasReference && !bypass){
            confidence=float(jobEstimate.confidence);locked=jobEstimate.valid;
            analysisReason=int(jobEstimate.reason);analysisChannel=jobChannel;
            if(mode==0){beforeScore=phasetwin::coherenceScore(jobBeforeCorrelation);alignmentScore=phasetwin::coherenceScore(jobPostCorrelation);}
            if(mode==0 && (learnActive || automatic)){
                learning.add(jobEstimate);
                const auto candidate=learning.result(sampleRateHz,mode==1);reliability=float(candidate.confidence);
                if(automatic && !learnActive && !freeze && candidate.reliable){
                    if(values[9]->load()>0.5f && std::abs(candidate.lag-detectedLag.load())>0.05)detectedLag=float(candidate.lag);
                    if(values[10]->load()>0.5f)inverted=candidate.inverted;
                }
            }
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
            if(!kickConfig.allowTiming && !kickConfig.allowPolarity)learnState=4;
            else if(kickResult.reliable){commitLearned(kickConfig.allowTiming?kickResult.lag*sampleRateHz/phasetwin::kickRate:detectedLag.load(),kickConfig.allowPolarity?kickResult.inverted:inverted.load());learnState=2;}else learnState=3;
            learnActive=false;learnProgress=1;
        }
        kickMailbox.store(0,std::memory_order_release);if(learnActive)kickStarted=false;
    }
    if(learnActive && (freeze || bypass || !hasReference)){learnActive=false;learnState=3;}
    if(learnActive && mode==1 && !kickStarted && kickMailbox.load(std::memory_order_acquire)==0){
        kickData.count=0;kickCapture.prepare(sampleRateHz,band);kickStarted=true;
        kickConfig={std::clamp(int(phasetwin::kickRate*values[2]->load()/1000),1,120),detectedLag.load()*phasetwin::kickRate/sampleRateHz,trim*phasetwin::kickRate/1000.0,inverted.load(),polarity>0.5f,values[9]->load()>0.5f,values[10]->load()>0.5f};
        kickGeneration=generation;
    }
    if(mode==0 && learnActive && learnTimedOut && mailbox.load(std::memory_order_acquire)!=1){
        const auto candidate=learning.result(sampleRateHz,mode==1);reliability=float(candidate.confidence);
        if(values[9]->load()<0.5f && values[10]->load()<0.5f)learnState=4;
        else if(candidate.reliable && !freeze){
            commitLearned(values[9]->load()>0.5f?candidate.lag:detectedLag.load(),values[10]->load()>0.5f?candidate.inverted:inverted.load());
            learnState=(values[9]->load()>0.5f || values[10]->load()>0.5f)?2:4;
        }else learnState=3;
        learnActive=false;
    }
    double lag=double(detectedLag.load())+sampleRateHz*trim/1000.0;
    lag=bypass?0.0:std::clamp(lag,-double(engine.getMaximumLag()),double(engine.getMaximumLag()));
    const bool invert=!bypass && (inverted.load()!=(polarity>0.5f));
    if(std::abs(lag-previousLag)>0.001 || invert!=previousInvert){
        ++generation;captureIndex=0;captureSettled=true;kickStarted=false;
        verified=false;verificationAvailable=false;previousLag=lag;previousInvert=invert;
    }
    engine.setCorrection(lag,invert);
    referenceMix.setTargetValue(bypass?0.0f:values[6]->load());outputGain.setTargetValue(bypass?1.0f:juce::Decibels::decibelsToGain(values[7]->load()));
    compareMix.setTargetValue(values[12]->load()>0.5f?1.0f:0.0f);
    scopePacket.sampleRate=sampleRateHz;scopePacket.generation=scopeGeneration;
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
        if(learnActive && mode==1 && kickStarted && kickMailbox.load(std::memory_order_relaxed)==0){
            if(kickCapture.push(kickData,a,mappedB))kickMailbox.store(1,std::memory_order_release);
            learnProgress=float(kickData.count)/phasetwin::kickCaptureSize;
        }
        if(learnActive && mode==0 && !learnTimedOut){++learnElapsed;learnProgress=float(std::min(1.0,learnElapsed/(2*sampleRateHz)));if(learnElapsed>=std::uint64_t(2*sampleRateHz))learnTimedOut=true;}
        inputEnergyA+=0.5*(double(a[0])*a[0]+double(a[1])*a[1]);
        inputEnergyB+=0.5*(double(b[0])*b[0]+double(b[1])*b[1]);
        if(hasReference && !bypass){
            for(int ch=0;ch<2;++ch){captureA[ch][captureIndex]=a[ch];captureB[ch][captureIndex]=mappedB[ch];
                capturePostA[ch][captureIndex]=corrected.a[ch];capturePostB[ch][captureIndex]=corrected.b[ch];}
            captureSettled=captureSettled && !engine.isSettling() && !compareMix.isSmoothing();
            if(++captureIndex==phasetwin::frameSize){
                captureIndex=0;
                if(mailbox.load(std::memory_order_acquire)==0){
                    jobA=captureA;jobB=captureB;jobPostA=capturePostA;jobPostB=capturePostB;
                    jobSearch=search;jobGate=gate;jobGeneration=generation;jobWasSettled=captureSettled;jobLowEnd=mode==1;jobSampleRate=sampleRateHz;jobCutoff=band;
                    mailbox.store(1,std::memory_order_release);
                }
                captureSettled=true;
            }
        }else captureIndex=0;
        if(scopeIndex==0)scopePacket.firstSample=scopeSampleClock;
        ++scopeSampleClock;
        for(int ch=0;ch<2;++ch){
            scopePacket.traces[ch][scopeIndex]=a[ch];scopePacket.traces[2+ch][scopeIndex]=mappedB[ch];
            scopePacket.traces[4+ch][scopeIndex]=corrected.a[ch];scopePacket.traces[6+ch][scopeIndex]=corrected.b[ch];
        }
        if(++scopeIndex==phasetwin::scopePacketSamples){scopeIndex=0;scope.push(scopePacket);}
        const float mix=referenceMix.getNextValue(),gain=outputGain.getNextValue();
        for(int ch=0;ch<output.getNumChannels();++ch)output.setSample(ch,n,gain*(corrected.a[ch]+mix*corrected.b[ch]));
    }
    if(buffer.getNumSamples()>0){
        const float release=float(60.0*buffer.getNumSamples()/sampleRateHz);
        inputDbA=std::max(inputDbA.load()-release,float(10*std::log10(std::max(1e-10,inputEnergyA/buffer.getNumSamples()))));
        inputDbB=std::max(inputDbB.load()-release,float(10*std::log10(std::max(1e-10,inputEnergyB/buffer.getNumSamples()))));
    }
    appliedLag=float(engine.appliedLag());settling=engine.isSettling();
    if(engine.isSettling())verified=false;
    if(!hasReference || bypass){confidence=0;locked=false;verified=false;verificationAvailable=false;postCorrelation=0;alignmentScore=0;}
}
void PhaseTwinProcessor::getStateInformation(juce::MemoryBlock& block){auto state=parameters.copyState();state.setProperty("schema",4,nullptr);state.setProperty("scopeView",int(scopePreferences.load()),nullptr);state.setProperty("undoAvailable",undoAvailable.load(),nullptr);state.setProperty("priorLagMs",priorLagMs.load(),nullptr);state.setProperty("priorPolarity",priorPolarity.load(),nullptr);state.setProperty("heldLagMs",1000.0*detectedLag.load()/sampleRateHz,nullptr);state.setProperty("heldPolarity",inverted.load(),nullptr);if(auto xml=state.createXml())copyXmlToBinary(*xml,block);}
void PhaseTwinProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType())){auto state=juce::ValueTree::fromXml(*xml);scopePreferences=std::uint32_t(int(state.getProperty("scopeView",int(phasetwin::ScopePreferences{}.packed()))));scopeViewRevision.fetch_add(1);const double priorMs=double(state.getProperty("priorLagMs",0.0));priorLagMs=float(std::isfinite(priorMs)?std::clamp(priorMs,-40.0,40.0):0.0);priorPolarity=bool(state.getProperty("priorPolarity",false));undoAvailable=bool(state.getProperty("undoAvailable",false));const double heldMs=double(state.getProperty("heldLagMs",0.0));detectedLag.store(float((std::isfinite(heldMs)?std::clamp(heldMs,-40.0,40.0):0.0)*sampleRateHz/1000.0));inverted.store(bool(state.getProperty("heldPolarity",false)));if(int(state.getProperty("schema",0))<3){auto oldAuto=state.getChildWithProperty("id","auto");if(oldAuto.isValid() && double(oldAuto.getProperty("value",1.0))<0.5){detectedLag=0;inverted=false;}}
        parameters.replaceState(state);stateRevision.fetch_add(1);}}
juce::AudioProcessorEditor* PhaseTwinProcessor::createEditor(){return new PhaseTwinEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PhaseTwinProcessor();}
