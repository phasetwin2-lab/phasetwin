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
 require(std::abs(processor->getTailLengthSeconds()-1952.0/48000)<1e-9,"tail reporting incorrect");
 auto set=[&](const char* id,float value){auto* p=processor->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
 set("profile",1);set("gain",0);set("auto",0);
 juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
 std::int64_t clock=0;processor->alignRequest.fetch_add(1);
 auto render=[&]{audio.clear();for(int n=0;n<256;++n,++clock){auto wave=[](std::int64_t t){if(t<0)return 0.0f;auto phase=t%24000;return phase<10000?float(std::sin(2*juce::MathConstants<double>::pi*60*phase/48000.0)*std::exp(-double(phase)/2400)):0.0f;};float a=-wave(clock-192),b=wave(clock);audio.setSample(0,n,a);audio.setSample(1,n,a);audio.setSample(2,n,b);audio.setSample(3,n,b);}processor->processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n)require(std::isfinite(audio.getSample(c,n)),"nonfinite output");};
 for(int block=0;block<800;++block)render();
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
 while(processor->learnState.load()==1 && std::chrono::steady_clock::now()<deadline){render();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 require(processor->learnState.load()==2,"kick learning did not apply");
 require(processor->inverted.load(),"kick polarity incorrect");
 require(std::abs(processor->detectedLag.load()-192)<16,"kick timing incorrect");
 require(processor->undoAvailable.load(),"successful learn did not create undo history");
 phasetwin::ScopePreferences view;view.style=1;view.division=7;view.channel=1;processor->scopePreferences=view.packed();
 juce::MemoryBlock state;processor->getStateInformation(state);
 auto restored=std::make_unique<PhaseTwinProcessor>();restored->setStateInformation(state.getData(),int(state.getSize()));restored->setRateAndBufferSizeDetails(96000,256);restored->prepareToPlay(96000,256);
 require(restored->inverted.load(),"polarity state lost");require(std::abs(restored->detectedLag.load()-processor->detectedLag.load()*2)<.01,"sample rate state scaling incorrect");
 require(restored->scopePreferences.load()==view.packed(),"scope preferences not recalled");require(restored->undoAvailable.load(),"undo history not recalled");
 const auto learned=processor->detectedLag.load();processor->alignRequest.fetch_add(1);render();require(processor->learnState.load()==1,"analysis did not restart");processor->cancelRequest.fetch_add(1);render();require(processor->learnState.load()==6 && processor->detectedLag.load()==learned,"cancel changed correction or left learning active");
 set("manual",1);processor->undoRequest.fetch_add(1);render();require(processor->learnState.load()==5 && processor->detectedLag.load()==0 && !processor->inverted.load(),"undo did not restore prior learned correction");require(processor->parameters.getRawParameterValue("manual")->load()==1,"undo changed manual trim");
 PhaseTwinScope graph;graph.setSize(900,280);graph.setTriggered(false);graph.setWindow(20,"native test");phasetwin::ScopePacket packet;
 for(int k=0;k<8;++k){packet.firstSample=std::uint64_t(k*phasetwin::scopePacketSamples);for(int n=0;n<phasetwin::scopePacketSamples;++n)for(int t=0;t<8;++t)packet.traces[t][n]=float(.7*std::sin(.025*(k*phasetwin::scopePacketSamples+n)+t*.4));graph.ingest(packet);}
 juce::Image lines(juce::Image::ARGB,900,280,true),filled(juce::Image::ARGB,900,280,true);juce::Graphics lineGraphics(lines),fillGraphics(filled);graph.setFilled(false);graph.paint(lineGraphics);graph.setFilled(true);graph.paint(fillGraphics);int differingPixels=0;for(int y=0;y<280;++y)for(int x=0;x<900;++x)differingPixels+=lines.getPixelAt(x,y)!=filled.getPixelAt(x,y);require(differingPixels>1000,"filled rendering did not change waveform area");
juce::Image sumImage(juce::Image::ARGB,900,280,true);juce::Graphics sumGraphics(sumImage);graph.setSummed(true);graph.setVisibleWaves(false,false);graph.paint(sumGraphics);int sumDifferences=0;for(int y=0;y<280;++y)for(int x=0;x<900;++x)sumDifferences+=filled.getPixelAt(x,y)!=sumImage.getPixelAt(x,y);require(sumDifferences>1000,"summed scope did not render independently of trace visibility");
 restored->releaseResources();processor->releaseResources();
 std::cout<<"PASS native learning, finite output, state restore and rate scaling\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
