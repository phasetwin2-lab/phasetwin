#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 juce::ScopedJuceInitialiser_GUI gui;
 auto processor=std::make_unique<PhaseTwinProcessor>();
 require(!processor->isBusesLayoutSupported({}),"invalid layout accepted");
 processor->setRateAndBufferSizeDetails(48000,256);processor->prepareToPlay(48000,256);
 require(processor->getLatencySamples()==992,"fractional interpolation latency guard missing");
 require(std::abs(processor->getTailLengthSeconds()-(.3+1952.0/48000))<1e-9,"tail reporting incorrect");
 auto set=[&](const char* id,float value){auto* p=processor->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
 require(processor->parameters.getRawParameterValue("correctionMode")->load()==0,"automatic correction default incorrect");
 set("profile",1);set("gain",0);set("auto",0);set("preserveGroove",0);
 juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
 std::int64_t clock=0;processor->alignRequest.fetch_add(1);
 auto render=[&]{audio.clear();for(int n=0;n<256;++n,++clock){auto wave=[](std::int64_t t){if(t<0)return 0.0f;auto phase=t%24000;return phase<10000?float(std::sin(2*juce::MathConstants<double>::pi*60*phase/48000.0)*std::exp(-double(phase)/2400)):0.0f;};float a=-wave(clock-192),b=wave(clock);audio.setSample(0,n,a);audio.setSample(1,n,a);audio.setSample(2,n,b);audio.setSample(3,n,b);}processor->processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n)require(std::isfinite(audio.getSample(c,n)),"nonfinite output");};
 for(int block=0;block<800;++block)render();
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
 while(processor->learnState.load()==1 && std::chrono::steady_clock::now()<deadline){render();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 require(processor->learnState.load()==7 && processor->recommendationReady.load(),"kick preview missing");
 require(processor->detectedLag.load()==0 && !processor->inverted.load(),"preview applied before approval");
 set("audition",3);set("compare",1);processor->captureHistory();const auto previewUndoCount=processor->undoSteps.load();processor->hearProposalRequested=true;for(int block=0;block<25;++block)render();
 require(processor->hearingProposal.load() && processor->recommendationReady.load(),"proposal audition did not start");require(std::abs(processor->appliedLag.load()-processor->proposedLagMs.load()*48)<.01f,"proposal audition did not override component timing");require(processor->detectedLag.load()==0 && !processor->inverted.load() && processor->undoSteps.load()==previewUndoCount,"temporary audition changed held correction or undo");
 processor->hearProposalRequested=false;for(int block=0;block<25;++block)render();require(!processor->hearingProposal.load() && std::abs(processor->appliedLag.load())<.01f,"proposal off did not restore held audition");require(processor->parameters.getRawParameterValue("audition")->load()==3 && processor->parameters.getRawParameterValue("compare")->load()==1,"temporary audition overwrote stored choices");
 processor->hearProposalRequested=true;render();processor->applyRequest.fetch_add(1);render();require(processor->learnState.load()==2 && !processor->hearingProposal.load(),"kick recommendation did not apply/end audition");processor->captureHistory();const auto appliedProposalLag=processor->detectedLag.load();processor->undoAudio();render();require(processor->detectedLag.load()==0 && !processor->inverted.load(),"applied recommendation undo failed");processor->redoAudio();render();require(processor->detectedLag.load()==appliedProposalLag && processor->inverted.load(),"applied recommendation redo failed");set("audition",0);set("compare",0);render();
 require(processor->inverted.load(),"kick polarity incorrect");
 require(std::abs(processor->detectedLag.load()-192)<16,"kick timing incorrect");
 processor->captureHistory();require(processor->undoAvailable.load(),"successful learn did not create undo history");
 const float preservedLag=processor->detectedLag.load();set("correctionMode",2);render();require(processor->detectedLag.load()==preservedLag,"preserve timing cleared held offset");
 set("correctionMode",1);render();require(processor->inverted.load(),"preserve polarity cleared held inversion");
 phasetwin::ScopePreferences view;view.style=1;view.division=7;view.channel=1;view.postDuck=true;processor->scopePreferences=view.packed();processor->editorSections=21;
 juce::MemoryBlock state;processor->getStateInformation(state);
 auto restored=std::make_unique<PhaseTwinProcessor>();restored->setStateInformation(state.getData(),int(state.getSize()));restored->setRateAndBufferSizeDetails(96000,256);restored->prepareToPlay(96000,256);
 require(restored->parameters.getRawParameterValue("correctionMode")->load()==1,"correction mode state lost");
 require(restored->inverted.load(),"polarity state lost");require(std::abs(restored->detectedLag.load()-processor->detectedLag.load()*2)<.01,"sample rate state scaling incorrect");
 require(restored->editorSections.load()==21,"collapsed section state not recalled");
 require(restored->scopePreferences.load()==view.packed(),"scope preferences not recalled");restored->captureHistory();require(!restored->undoAvailable.load() && !restored->redoAvailable.load(),"state recall did not start fresh history");
 const auto learned=processor->detectedLag.load();processor->alignRequest.fetch_add(1);render();require(processor->learnState.load()==1,"analysis did not restart");processor->cancelRequest.fetch_add(1);render();require(processor->learnState.load()==6 && processor->detectedLag.load()==learned,"cancel changed correction or left learning active");
 processor->captureHistory();set("manual",1);processor->undoAudio();render();require(processor->learnState.load()==5 && processor->detectedLag.load()==learned && processor->inverted.load(),"undo manual edit changed learned correction");require(processor->parameters.getRawParameterValue("manual")->load()==0,"undo did not restore manual trim");processor->redoAudio();render();require(processor->parameters.getRawParameterValue("manual")->load()==1,"redo did not restore manual trim");
 // Native history integration: grouping, display exclusions, reset, preview and branching.
 auto edits=std::make_unique<PhaseTwinProcessor>();edits->setRateAndBufferSizeDetails(48000,256);edits->prepareToPlay(48000,256);edits->captureHistory();
 auto edit=[&](const char* id,float value){auto* p=edits->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
 auto renderEdits=[&]{audio.clear();edits->processBlock(audio,midi);edits->captureHistory();};
 auto* gainEdit=edits->parameters.getParameter("gain");gainEdit->beginChangeGesture();for(int k=1;k<=50;++k){edit("gain",float(-k)/10);edits->captureHistory();}gainEdit->endChangeGesture();require(edits->undoSteps.load()==1,"slider drag created multiple steps");
 edits->beginHistoryAction();edit("duckEnabled",1);edit("duckAmount",80);edit("duckRelease",200);edits->endHistoryAction();require(edits->undoSteps.load()==2,"grouped preset fragmented");
 edits->scopePreferences=0;edits->editorSections=31;edit("preview",0);edits->captureHistory();require(edits->undoSteps.load()==2,"display/workflow changed history");
 edits->undoAudio();renderEdits();require(edits->parameters.getRawParameterValue("duckEnabled")->load()==0 && edits->parameters.getRawParameterValue("duckAmount")->load()==50,"preset undo incomplete");require(edits->scopePreferences.load()==0 && edits->editorSections.load()==31 && edits->parameters.getRawParameterValue("preview")->load()==0,"undo changed display/workflow");
 edits->undoAudio();renderEdits();require(edits->parameters.getRawParameterValue("gain")->load()==0,"second undo failed");edits->redoAudio();renderEdits();edits->redoAudio();renderEdits();require(edits->parameters.getRawParameterValue("gain")->load()==-5 && edits->parameters.getRawParameterValue("duckEnabled")->load()==1,"multiple redo failed");
 juce::MemoryBlock editState;edits->getStateInformation(editState);auto editXml=juce::AudioProcessor::getXmlFromBinary(editState.getData(),int(editState.getSize()));auto editTree=juce::ValueTree::fromXml(*editXml);editTree.setProperty("heldLagMs",1.125,nullptr);editTree.setProperty("heldPolarity",true,nullptr);editXml=editTree.createXml();juce::AudioProcessor::copyXmlToBinary(*editXml,editState);edits->setStateInformation(editState.getData(),int(editState.getSize()));renderEdits();const auto beforeResetSteps=edits->undoSteps.load();require(beforeResetSteps==0,"recall history not cleared");
 edits->beginHistoryAction();edit("manual",2);edit("polarity",1);edits->endHistoryAction();
 edits->beginHistoryAction();edit("manual",0);edit("polarity",0);edits->resetRequest.fetch_add(1);edits->endHistoryAction();renderEdits();require(edits->undoSteps.load()==beforeResetSteps+2,"reset fragmented or missing");edits->undoAudio();renderEdits();require(edits->detectedLag.load()==54 && edits->inverted.load() && edits->parameters.getRawParameterValue("manual")->load()==2 && edits->parameters.getRawParameterValue("polarity")->load()==1,"reset undo incomplete");
 edits->undoAudio();renderEdits();edits->redoAudio();renderEdits();edit("rotation",1);require(!edits->redoAvailable.load(),"new audio edit retained redo branch");
 edits->undoAudio();edits->undoAudio();edits->redoAudio();renderEdits();require(edits->detectedLag.load()==54 && edits->inverted.load() && edits->parameters.getRawParameterValue("rotation")->load()==0,"stopped undo/redo ordering failed");
 edits->undoAudio();edit("gain",-3);edits->undoAudio();renderEdits();require(edits->parameters.getRawParameterValue("gain")->load()==-5,"edit during stopped restore not undoable");
 // Optional collected-section workflow plus fresh post-Apply verification.
 auto sessions=std::make_unique<PhaseTwinProcessor>();sessions->setRateAndBufferSizeDetails(48000,256);sessions->prepareToPlay(48000,256);
 auto sessionSet=[&](const char* id,float value){auto* p=sessions->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};sessionSet("preserveGroove",0);sessionSet("multiSection",1);
 std::int64_t sectionClock=0;double sectionFrequency=60;
 auto renderSession=[&]{for(int n=0;n<256;++n,++sectionClock){auto wave=[&](std::int64_t t){if(t<0)return 0.0f;auto phase=t%24000;return phase<10000?float(std::sin(2*juce::MathConstants<double>::pi*sectionFrequency*phase/48000)*std::exp(-double(phase)/2400)):0.0f;};for(int ch=0;ch<4;++ch)audio.setSample(ch,n,ch<2?-wave(sectionClock-192):wave(sectionClock));}sessions->processBlock(audio,midi);};
 for(int section=0;section<2;++section){sectionFrequency=60+20*section;sessions->alignRequest.fetch_add(1);for(int k=0;k<800;++k)renderSession();const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);while((sessions->learnState.load()==1 || sessions->learnState.load()==10) && std::chrono::steady_clock::now()<until){renderSession();std::this_thread::sleep_for(std::chrono::milliseconds(2));}require(sessions->sectionCount.load()==section+1 && sessions->learnState.load()==9,"section capture failed");require(sessions->detectedLag.load()==0 && !sessions->recommendationReady.load(),"collect applied/proposed early");}
 sessions->evaluateSectionsRequest.fetch_add(1);renderSession();const auto evaluateUntil=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(sessions->learnState.load()==10 && std::chrono::steady_clock::now()<evaluateUntil){renderSession();std::this_thread::sleep_for(std::chrono::milliseconds(2));}require(sessions->recommendationReady.load() && sessions->sectionCount.load()==2,"common section proposal missing");
 require(sessions->sectionGains[0].load()>.02f && sessions->sectionGains[1].load()>.02f,"common proposal did not improve both sections");sessions->applyRequest.fetch_add(1);renderSession();require(sessions->learnState.load()==2 && sessions->sectionCount.load()==0,"section apply did not commit/clear collection");require(sessions->verifyState.load()==1 || sessions->verifyState.load()==2,"post apply check not scheduled");
 const float heldAfterApply=sessions->detectedLag.load();for(int k=0;k<1000;++k)renderSession();const auto checkUntil=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(sessions->verifyState.load()<3 && std::chrono::steady_clock::now()<checkUntil){renderSession();std::this_thread::sleep_for(std::chrono::milliseconds(2));}require(sessions->verifyState.load()==3 && sessions->verifyAfter.load()>sessions->verifyBefore.load()+.02f,"fresh check did not detect improvement");require(sessions->detectedLag.load()==heldAfterApply && sessions->inverted.load(),"verification modified correction");sessions->captureHistory();const auto stepsBeforeWorkflow=sessions->undoSteps.load();sessionSet("verifyAfterApply",0);sessionSet("multiSection",0);renderSession();sessions->captureHistory();require(sessions->verifyState.load()==0 && sessions->undoSteps.load()==stepsBeforeWorkflow,"workflow options affected history or failed to disable check");
 juce::MemoryBlock sessionState;sessions->getStateInformation(sessionState);auto sessionRecall=std::make_unique<PhaseTwinProcessor>();sessionRecall->setStateInformation(sessionState.getData(),int(sessionState.getSize()));require(sessionRecall->parameters.getRawParameterValue("verifyAfterApply")->load()==0 && sessionRecall->sectionCount.load()==0,"session options or transient capture recalled incorrectly");
 PhaseTwinScope graph;graph.setSize(900,280);graph.setTriggered(false);graph.setWindow(20,"native test");phasetwin::ScopePacket packet;
 for(int k=0;k<8;++k){packet.firstSample=std::uint64_t(k*phasetwin::scopePacketSamples);for(int n=0;n<phasetwin::scopePacketSamples;++n)for(int t=0;t<8;++t)packet.traces[t][n]=float(.7*std::sin(.025*(k*phasetwin::scopePacketSamples+n)+t*.4));graph.ingest(packet);}
 juce::Image lines(juce::Image::ARGB,900,280,true),filled(juce::Image::ARGB,900,280,true);juce::Graphics lineGraphics(lines),fillGraphics(filled);graph.setFilled(false);graph.paint(lineGraphics);graph.setFilled(true);graph.paint(fillGraphics);int differingPixels=0;for(int y=0;y<280;++y)for(int x=0;x<900;++x)differingPixels+=lines.getPixelAt(x,y)!=filled.getPixelAt(x,y);require(differingPixels>1000,"filled rendering did not change waveform area");
juce::Image sumImage(juce::Image::ARGB,900,280,true);juce::Graphics sumGraphics(sumImage);graph.setSummed(true);graph.setVisibleWaves(false,false);graph.paint(sumGraphics);int sumDifferences=0;for(int y=0;y<280;++y)for(int x=0;x<900;++x)sumDifferences+=filled.getPixelAt(x,y)!=sumImage.getPixelAt(x,y);require(sumDifferences>1000,"summed scope did not render independently of trace visibility");
 // Wrapper ducking regression: A alone is attenuated; summed B remains at unity.
 auto duck=std::make_unique<PhaseTwinProcessor>();duck->setRateAndBufferSizeDetails(48000,256);duck->prepareToPlay(48000,256);
 auto setDuck=[&](const char* id,float value){auto* p=duck->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
 setDuck("duckEnabled",1);setDuck("duckAmount",100);setDuck("duckHarshness",100);setDuck("mix",1);
 auto renderDuck=[&](float target,bool bypass){for(int c=0;c<4;++c)for(int n=0;n<256;++n)audio.setSample(c,n,c<2?target:.8f);if(bypass)duck->processBlockBypassed(audio,midi);else duck->processBlock(audio,midi);};
 for(int k=0;k<150;++k)renderDuck(.25f,false);
 require(std::abs(audio.getSample(0,255)-(.8f+.25f*std::pow(10.0f,-24.0f/20)))<.001f,"ducking affected reference or missed target");
 require(std::abs(audio.getSample(0,255)-audio.getSample(1,255))<1e-6f,"ducking stereo mismatch");
 require(std::abs(duck->duckReductionDb.load()-24)<.01f,"duck reduction meter incorrect");
 phasetwin::ScopePacket duckScope,lastDuckScope;bool capturedDuck=false;while(duck->scope.pop(duckScope)){lastDuckScope=duckScope;capturedDuck=true;}
 require(capturedDuck,"duck scope packets missing");require(std::abs(lastDuckScope.traces[8][255]-.25f*std::pow(10.0f,-24.0f/20))<.001f,"scope missed actual ducked A");require(std::abs(lastDuckScope.traces[6][255]-.8f)<.001f,"scope ducked reference B");require(std::abs(lastDuckScope.reductionDb[255]-24)<.01f,"scope reduction did not match actual gain");
 juce::MemoryBlock duckState;duck->getStateInformation(duckState);setDuck("duckEnabled",0);duck->setStateInformation(duckState.getData(),int(duckState.getSize()));
 require(duck->parameters.getRawParameterValue("duckEnabled")->load()==1 && duck->parameters.getRawParameterValue("duckAmount")->load()==100,"duck controls not recalled");
 for(int k=0;k<100;++k)renderDuck(.25f,true);
 require(std::abs(audio.getSample(0,255)-.25f)<.001f,"host bypass did not restore unducked target");
 auto legacy=juce::AudioProcessor::getXmlFromBinary(duckState.getData(),int(duckState.getSize()));require(legacy!=nullptr,"state XML missing");
 auto legacyTree=juce::ValueTree::fromXml(*legacy);for(auto* id:{"duckEnabled","duckAmount","duckHarshness"})legacyTree.removeChild(legacyTree.getChildWithProperty("id",id),nullptr);
 juce::MemoryBlock legacyState;auto legacyXml=legacyTree.createXml();juce::AudioProcessor::copyXmlToBinary(*legacyXml,legacyState);duck->setStateInformation(legacyState.getData(),int(legacyState.getSize()));
 require(duck->parameters.getRawParameterValue("duckEnabled")->load()==0,"legacy preset retained active ducking");
 require(duck->parameters.getRawParameterValue("duckAmount")->load()==50,"legacy duck amount default missing");
 for(int mask=0;mask<4;++mask){
  auto oldMode=legacyTree.createCopy();oldMode.removeChild(oldMode.getChildWithProperty("id","correctionMode"),nullptr);
  oldMode.getChildWithProperty("id","allowDelay").setProperty("value",(mask&1)?1:0,nullptr);oldMode.getChildWithProperty("id","allowPolarity").setProperty("value",(mask&2)?1:0,nullptr);
  auto oldXml=oldMode.createXml();juce::MemoryBlock oldBytes;juce::AudioProcessor::copyXmlToBinary(*oldXml,oldBytes);duck->setStateInformation(oldBytes.getData(),int(oldBytes.getSize()));
  require(duck->parameters.getRawParameterValue("correctionMode")->load()==((mask&1)?((mask&2)?0:1):2),"legacy correction permissions not migrated");
  if(mask==0)require(duck->parameters.getRawParameterValue("freeze")->load()==1,"legacy measure-only state not locked");
 }
 setDuck("duckEnabled",0);setDuck("mix",0);duck->detectedLag=96;duck->inverted=true;
 for(int mode=0;mode<4;++mode){setDuck("audition",float(mode));for(int block=0;block<100;++block)renderDuck(.25f,false);require(std::abs(duck->appliedLag.load()-((mode==2 || mode==3)?0:96))<.01f,"audition timing component wrong");require(std::abs(audio.getSample(0,255)-((mode==1 || mode==3)?.25f:-.25f))<.001f,"audition polarity component wrong");require(duck->detectedLag.load()==96 && duck->inverted.load(),"audition overwrote stored correction");}
 setDuck("audition",3);setDuck("mix",1);for(int block=0;block<100;++block)renderDuck(.8f,false);require(duck->outputClipped.load(),"output over-full-scale did not latch");require(std::abs(duck->outputPeakL.load()-20*std::log10(1.6f))<.01f && std::abs(duck->outputPeakR.load()-duck->outputPeakL.load())<.01f,"final output sample peak incorrect");
 setDuck("mix",0);for(int block=0;block<100;++block)renderDuck(.25f,false);require(duck->outputClipped.load(),"clip latch did not hold through safe output");duck->clearClipRequest.fetch_add(1);renderDuck(.25f,false);require(!duck->outputClipped.load(),"clip latch did not clear");
 setDuck("audition",0);setDuck("rotation",1);setDuck("rotationFrequency",80);setDuck("rotationQ",.707f);duck->detectedLag=0;duck->inverted=false;std::int64_t phaseClock=0;
 for(int block=0;block<400;++block){audio.clear();for(int n=0;n<256;++n,++phaseClock){float value=float(.25*std::sin(2*juce::MathConstants<double>::pi*80*phaseClock/48000));audio.setSample(0,n,value);audio.setSample(1,n,value);}duck->processBlock(audio,midi);}
 const float expected=float(-.25*std::sin(2*juce::MathConstants<double>::pi*80*(phaseClock-1-duck->getLatencySamples())/48000));require(std::abs(audio.getSample(0,255)-expected)<.001f,"wrapper phase rotation not applied at centre");
 juce::MemoryBlock rotatedState;duck->getStateInformation(rotatedState);setDuck("rotation",0);duck->setStateInformation(rotatedState.getData(),int(rotatedState.getSize()));require(duck->parameters.getRawParameterValue("rotation")->load()==1,"rotation state not restored");
 duck->releaseResources();
 PhaseTwinSpectrum spectralView(*processor);spectralView.setSize(900,320);processor->spectrumAvailable=true;for(int i=0;i<phasetwin::spectralBands;++i){processor->spectralBefore[i]=float(-150+i*4);processor->spectralAfter[i]=0;processor->spectralConfidence[i]=1;processor->spectralLevelDb[i]=-6;}spectralView.update();juce::Image continuous(juce::Image::ARGB,900,320,true),gapped(juce::Image::ARGB,900,320,true);juce::Graphics cg(continuous),gg(gapped);spectralView.paint(cg);processor->spectralConfidence[32]=0;spectralView.update();spectralView.paint(gg);int gaps=0;for(int y=0;y<320;++y)for(int x=0;x<900;++x)gaps+=continuous.getPixelAt(x,y)!=gapped.getPixelAt(x,y);require(gaps>20,"spectral unsupported band did not break rendering");
 restored->releaseResources();processor->releaseResources();
 std::cout<<"PASS native learning, finite output, state restore and rate scaling\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
