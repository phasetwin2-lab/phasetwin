#pragma once
#include "PluginProcessor.h"
#include "ScopeHistory.h"
#include "TriggeredScope.h"
#include "PluginTheme.h"
#include <functional>

class PhaseTwinScope final : public juce::Component {
    phasetwin::ScopeHistory rolling;
    phasetwin::TriggeredScope triggered;
    bool triggerMode=true,filled=true,showA=true,showB=true,summed=false,postDuck=false,duckEnabled=false;
    const phasetwin::ScopeHistory& history()const{return triggerMode?triggered.history():rolling;}
    int channel=0;
    double windowMs=125;
    bool paused=false;
    juce::String timing="DAW tempo";
    void panel(juce::Graphics& g,juce::Rectangle<float> bounds,int trace,const juce::String& title,int count,float scale) {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff162233),bounds.getX(),bounds.getY(),juce::Colour(0xff0e1724),bounds.getX(),bounds.getBottom(),false));g.fillRoundedRectangle(bounds,10);
        g.setColour(juce::Colour(0xff2e4058));g.drawRoundedRectangle(bounds.reduced(.5f),10,1);
        g.setColour(trace==0?juce::Colour(0xffffb86b):PhaseTwinTheme::accent());g.fillRoundedRectangle(bounds.getX()+12,bounds.getY()+12,3,14,1.5f);
        g.setColour(juce::Colour(0xffd3dce8));g.drawText(title,bounds.toNearestInt().reduced(22,10).removeFromTop(20),juce::Justification::left);
        auto plot=bounds.reduced(12,10);plot.removeFromTop(26);plot.removeFromBottom(30+(trace==4 && duckEnabled?42:0));
        g.setColour(juce::Colour(0xff25344a));
        for(int i=0;i<=4;++i){float y=plot.getY()+plot.getHeight()*float(i)/4;g.drawHorizontalLine(int(y),plot.getX(),plot.getRight());}
        g.setColour(juce::Colour(0xff465b75));g.drawHorizontalLine(int(plot.getCentreY()),plot.getX(),plot.getRight());
        g.setColour(juce::Colour(0xff25344a));
        for(int i=0;i<=4;++i){float x=plot.getX()+plot.getWidth()*float(i)/4;g.drawVerticalLine(int(x),plot.getY(),plot.getBottom());}
        if(count>1){
            const auto pixels=std::max(2,int(plot.getWidth()));
            for(int wave=0;wave<(summed?1:2);++wave){
                if(!summed && ((wave==0 && !showA) || (wave==1 && !showB)))continue;
                const auto colour=summed?juce::Colour(0xffb9a0ff):wave==0?juce::Colour(0xff72e9c6):juce::Colour(0xffffb86b);
                g.setColour(colour);
                juce::Path path;
                std::vector<std::pair<float,float>> envelope(filled?static_cast<std::size_t>(pixels):0);
                for(int x=0;x<pixels;++x){
                    const int begin=x*count/pixels,end=std::max(begin+1,(x+1)*count/pixels);
                    float lo=1e20f,hi=-1e20f;
                    auto range=history().range(summed?(trace==0?4:postDuck?7:5):(trace==4 && wave==0 && postDuck?6:trace/2+wave),channel,begin,std::min(end,count),count);lo=range.first;hi=range.second;
                    const auto signedBounds=phasetwin::filledBounds(lo,hi);if(filled)envelope[static_cast<std::size_t>(x)]=signedBounds;
                    const float px=plot.getX()+float(x)*plot.getWidth()/float(pixels-1);
                    const float py=plot.getCentreY()-0.5f*(lo+hi)*scale*plot.getHeight()*0.44f;
                    if(x==0)path.startNewSubPath(px,py);else path.lineTo(px,py);
                    // Preserve peaks when several samples fall into one screen pixel.
                    if(end-begin>1)g.drawLine(px,plot.getCentreY()-hi*scale*plot.getHeight()*0.44f,px,plot.getCentreY()-lo*scale*plot.getHeight()*0.44f,1);
                }
                if(filled){juce::Path area;
                    for(int x=0;x<pixels;++x){const float px=plot.getX()+float(x)*plot.getWidth()/float(pixels-1),y=plot.getCentreY()-envelope[static_cast<std::size_t>(x)].second*scale*plot.getHeight()*.44f;if(x==0)area.startNewSubPath(px,y);else area.lineTo(px,y);}
                    for(int x=pixels-1;x>=0;--x)area.lineTo(plot.getX()+float(x)*plot.getWidth()/float(pixels-1),plot.getCentreY()-envelope[static_cast<std::size_t>(x)].first*scale*plot.getHeight()*.44f);
                    area.closeSubPath();g.setColour(colour.withAlpha(.20f));g.fillPath(area);g.setColour(colour);
                }
                g.strokePath(path,juce::PathStrokeType(1.4f));
            }
        }
        if(summed && count>0){const auto sumRange=history().range(trace==0?4:postDuck?7:5,channel,0,count,count);if(std::max(std::abs(sumRange.first),std::abs(sumRange.second))<1e-5f){g.setColour(juce::Colour(0xffb5bcc9));g.drawText("Sum near zero · silence or cancellation",plot.toNearestInt(),juce::Justification::centred);}}
        if(trace==4 && duckEnabled && count>1){
            const auto graph=juce::Rectangle<float>(plot.getX(),plot.getBottom()+20,plot.getWidth(),25);
            g.setColour(juce::Colour(0xff091320));g.fillRoundedRectangle(graph,4);
            juce::Path reduction;const int pixels=std::max(2,int(graph.getWidth()));
            for(int x=0;x<pixels;++x){const int begin=x*count/pixels,end=std::max(begin+1,(x+1)*count/pixels);const auto range=history().range(8,0,begin,std::min(end,count),count);
                const float px=graph.getX()+float(x)*graph.getWidth()/float(pixels-1),py=graph.getY()+juce::jlimit(0.0f,24.0f,range.second)/24.0f*graph.getHeight();
                if(x==0)reduction.startNewSubPath(px,py);else reduction.lineTo(px,py);
            }
            g.setColour(juce::Colour(0xffb9a0ff));g.strokePath(reduction,juce::PathStrokeType(1.2f));
            const auto graphFont=g.getCurrentFont();g.setFont(juce::Font(juce::FontOptions(10.0f)));g.drawText("DUCK ENVELOPE · reduction 0–24 dB",graph.toNearestInt().reduced(4,0),juce::Justification::centredRight);g.setFont(graphFont);
        }
        g.setColour(juce::Colour(0xff9ba9ba));
        const auto previousFont=g.getCurrentFont();g.setFont(juce::Font(juce::FontOptions(10.0f)));
        for(int i=0;i<=4;++i){
            const float x=plot.getX()+plot.getWidth()*float(i)/4;
            const auto label=juce::String(1000.0*count/history().sampleRate()*(triggerMode?i/4.0:-(1-i/4.0)),1)+" ms";
            const float left=i==0?x:i==4?x-65:x-32;
            g.drawText(label,juce::Rectangle<float>(left,plot.getBottom()+2,65,12).toNearestInt(),i==0?juce::Justification::left:i==4?juce::Justification::right:juce::Justification::centred);
        }
        g.setFont(previousFont);
        g.drawText(juce::String(1000.0*count/history().sampleRate(),1)+" ms  |  "+timing+(triggerMode?"  |  reference-triggered / last complete capture":"  |  rolling")+"  |  shared scale"+(paused?"  |  PAUSED":""),bounds.toNearestInt().reduced(10).removeFromBottom(16),juce::Justification::right);
    }
public:
    void setPostDuck(bool state){postDuck=state;repaint();}
    void setDuckingEnabled(bool state){duckEnabled=state;repaint();}
    void setSummed(bool state){summed=state;repaint();}
    void setFilled(bool state){filled=state;repaint();}
    void setVisibleWaves(bool a,bool b){showA=a;showB=b;repaint();}
    void setChannel(int choice){channel=std::clamp(choice,0,2);triggered.setChannel(channel);repaint();}
    void setWindow(double ms,const juce::String& source){windowMs=ms;triggered.setWindow(ms);timing=source;repaint();}
    void clearIfLive(){if(!paused && !triggerMode)rolling.clear();repaint();}
    void setTriggered(bool state){triggerMode=state;triggered.cancel();rolling.clear();repaint();}
    void setPaused(bool state){paused=state;triggered.cancel();repaint();}
    void ingest(const phasetwin::ScopePacket& packet){if(!paused){if(triggerMode)triggered.ingest(packet);else rolling.ingest(packet);}}
    void paint(juce::Graphics& g)override{
        const int count=history().availableSamples(triggerMode?triggered.displayedWindowMs():windowMs);
        float peak=0.00001f;
        for(int wave=summed?4:0;wave<(summed?6:4);++wave){auto range=history().range(wave,channel,0,count,count);peak=std::max(peak,std::max(std::abs(range.first),std::abs(range.second)));}
        // Include both pre/post histories regardless of selected view: toggling cannot rescale.
        for(int wave: summed?std::array<int,2>{5,7}:std::array<int,2>{2,6}){auto range=history().range(wave,channel,0,count,count);peak=std::max(peak,std::max(std::abs(range.first),std::abs(range.second)));}
        auto bounds=getLocalBounds().toFloat();auto top=bounds.removeFromTop((bounds.getHeight()-8)/2);bounds.removeFromTop(8);
        panel(g,top,0,summed?"BEFORE SUM  /  A + B at unity":"BEFORE  /  A input + B input",count,1/peak);
        panel(g,bounds,4,juce::String(summed?"AFTER SUM  /  ":"AFTER  /  ")+(postDuck?"post-duck A + B":"pre-duck A + B")+" · before mix / gain",count,1/peak);
        if(!summed && !showA && !showB){g.setColour(juce::Colours::white);g.drawText("Enable A or B to view a waveform",getLocalBounds(),juce::Justification::centred);}
        else if(count==0){g.setColour(juce::Colours::white);g.drawText((triggerMode?"Waiting for a transient on B and a complete capture":"Play audio to see both waveforms"),getLocalBounds(),juce::Justification::centred);}
    }
};

class PhaseTwinSpectrum final : public juce::Component,public juce::SettableTooltipClient {
    PhaseTwinProcessor& processor;phasetwin::SpectralFrame data;bool available=false,held=false;int hover=-1;
    juce::ToggleButton before{"Before"},after{"After"};
    juce::Rectangle<float> plotBounds()const{return getLocalBounds().toFloat().withTrimmedLeft(54).withTrimmedRight(18).withTrimmedTop(49).withTrimmedBottom(65);}
    float xFor(double frequency)const{auto plot=plotBounds();return plot.getX()+float(std::log(frequency/20)/std::log(1000.0))*plot.getWidth();}
public:
    explicit PhaseTwinSpectrum(PhaseTwinProcessor& p):processor(p){setName("Spectral phase curves, before and after");addAndMakeVisible(before);addAndMakeVisible(after);before.setToggleState(true,juce::dontSendNotification);after.setToggleState(true,juce::dontSendNotification);before.onClick=after.onClick=[this]{repaint();};setTooltip("Supported bands are connected only across adjacent measurements without a phase wrap. Shading is relative joint energy; band support is not temporal coherence. Hold view freezes the snapshot.");}
    void resized()override{before.setBounds(getWidth()-205,7,95,26);after.setBounds(getWidth()-110,7,95,26);}
    void setPaused(bool pause){held=pause;if(!pause)update();repaint();}
    void update(){if(held)return;const auto epoch=processor.spectralEpoch.load(std::memory_order_acquire);if(epoch & 1u)return;phasetwin::SpectralFrame next;next.sampleRate=processor.spectralSampleRate.load();
        for(int i=0;i<phasetwin::spectralBands;++i){next.before[i]=processor.spectralBefore[i].load();next.after[i]=processor.spectralAfter[i].load();next.confidence[i]=processor.spectralConfidence[i].load();next.levelDb[i]=processor.spectralLevelDb[i].load();}
        const bool valid=processor.spectrumAvailable.load();if(epoch!=processor.spectralEpoch.load(std::memory_order_acquire))return;data=next;available=valid;repaint();}
    void mouseMove(const juce::MouseEvent& e)override{auto plot=plotBounds();hover=plot.contains(e.position)?juce::jlimit(0,phasetwin::spectralBands-1,int((e.position.x-plot.getX())/plot.getWidth()*phasetwin::spectralBands)):-1;repaint();}
    void mouseExit(const juce::MouseEvent&)override{hover=-1;repaint();}
    void paint(juce::Graphics& g)override{
        g.setColour(juce::Colour(0xff101a28));g.fillRoundedRectangle(getLocalBounds().toFloat(),10);auto plot=plotBounds();
        g.setFont(juce::Font(juce::FontOptions(12.0f)));g.setColour(juce::Colour(0xffc5cfdf));g.drawText("SPECTRAL PHASE · auto channel"+juce::String(held?" · HELD":""),12,8,getWidth()-230,20,juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(10.0f)));g.setColour(juce::Colour(0xff8fa1ba));g.drawText("Amber: before   Mint: after   Shading: relative joint energy   Gaps: weak support / phase wrap",12,29,getWidth()-24,16,juce::Justification::centredLeft);
        bool supported=false;
        if(available)for(int i=0;i<phasetwin::spectralBands;++i)if(data.confidence[i]>=.25f){supported=true;const float x=plot.getX()+float(i)*plot.getWidth()/phasetwin::spectralBands,w=plot.getWidth()/phasetwin::spectralBands;g.setColour(PhaseTwinTheme::accent().withAlpha(.025f+.10f*juce::jlimit(0.0f,1.0f,(data.levelDb[i]+30)/30)));g.fillRect(x,plot.getY(),w,plot.getHeight());}
        for(int i=0;i<5;++i){float y=plot.getY()+i*plot.getHeight()/4;g.setColour(juce::Colour(i==2?0xff6f899d:0xff30425b));g.drawHorizontalLine(int(y),plot.getX(),plot.getRight());g.setColour(juce::Colour(0xffa2b1c7));g.drawText(juce::String(180-i*90)+"°",2,int(y)-8,44,16,juce::Justification::centredRight);}
        for(double frequency:{20.0,50.0,100.0,200.0,500.0,1000.0,2000.0,5000.0,10000.0,20000.0}){const float x=xFor(frequency);g.setColour(juce::Colour(0xff26374b));g.drawVerticalLine(int(x),plot.getY(),plot.getBottom());g.setColour(juce::Colour(0xffa2b1c7));g.drawText(frequency>=1000?juce::String(frequency/1000,0)+"k":juce::String(int(frequency)),int(x)-22,int(plot.getBottom())+4,44,16,juce::Justification::centred);}
        if(available)for(int trace=0;trace<2;++trace){if(!(trace==0?before.getToggleState():after.getToggleState()))continue;const auto& degrees=trace==0?data.before:data.after;const auto colour=trace==0?juce::Colour(0xffffb86b):PhaseTwinTheme::accent();
            for(int i=0;i<phasetwin::spectralBands;++i){const float support=data.confidence[i];if(support<.25f)continue;const float x=plot.getX()+(i+.5f)*plot.getWidth()/phasetwin::spectralBands,y=plot.getCentreY()-degrees[i]/180*plot.getHeight()/2;
                g.setColour(colour.withAlpha(.35f+.65f*support));if(i>0 && phasetwin::phaseCurveConnects(degrees[i-1],degrees[i],data.confidence[i-1],support)){const float px=plot.getX()+(i-.5f)*plot.getWidth()/phasetwin::spectralBands,py=plot.getCentreY()-degrees[i-1]/180*plot.getHeight()/2;g.drawLine(px,py,x,y,1.8f);}g.fillEllipse(x-1.5f,y-1.5f,3,3);
            }
        }
        juce::String readout="8192-sample FFT · bin spacing "+juce::String(data.sampleRate/phasetwin::frameSize,2)+" Hz · support is NOT coherence";
        if(hover>=0){const double lo=20*std::pow(1000.0,double(hover)/phasetwin::spectralBands),hi=20*std::pow(1000.0,double(hover+1)/phasetwin::spectralBands);const float x=plot.getX()+(hover+.5f)*plot.getWidth()/phasetwin::spectralBands;g.setColour(juce::Colour(0xff91a2b8));g.drawVerticalLine(int(x),plot.getY(),plot.getBottom());readout=juce::String(lo,1)+"–"+juce::String(hi,1)+" Hz · ";if(available && data.confidence[hover]>=.25f)readout+="Before "+juce::String(data.before[hover],1)+"° · After "+juce::String(data.after[hover],1)+"° · band support "+juce::String(data.confidence[hover]*100,0)+"% · joint energy "+juce::String(data.levelDb[hover],1)+" dB relative";else readout+="insufficient supported evidence";}
        g.setColour(juce::Colour(0xffc5cfdf));g.drawText(readout,12,getHeight()-27,getWidth()-24,20,juce::Justification::centredLeft);
        if(!available || !supported){g.setColour(juce::Colour(0xffe3eaf5));g.drawText(available?"No supported bands · play a stronger related passage":"Play both signals for spectral evidence",plot.toNearestInt(),juce::Justification::centred);}
    }
};

class PhaseTwinOutputMeter final : public juce::Component,public juce::SettableTooltipClient {
    PhaseTwinProcessor& processor;float left=-100,right=-100;bool clipped=false;
public:
    explicit PhaseTwinOutputMeter(PhaseTwinProcessor& p):processor(p){setTooltip("Final L/R sample peaks after ducking, reference mix and gain. 0 dBFS marker. CLIP latches at full scale; click to clear on next audio callback. Does not detect intersample peaks or limit output.");setName("Output sample peaks; click to clear clip latch");setMouseCursor(juce::MouseCursor::PointingHandCursor);setWantsKeyboardFocus(true);}
    bool keyPressed(const juce::KeyPress& key)override{if(key==juce::KeyPress::spaceKey || key==juce::KeyPress::returnKey){processor.clearClipRequest.fetch_add(1);return true;}return false;}
    void update(float l,float r,bool clip){left=l;right=r;clipped=clip;repaint();}
    void mouseDown(const juce::MouseEvent&)override{processor.clearClipRequest.fetch_add(1);}
    void paint(juce::Graphics& g)override{
        g.setFont(juce::Font(juce::FontOptions(10.0f)));const float width=float(getWidth()-55);
        for(int ch=0;ch<2;++ch){const float db=ch==0?left:right,y=ch==0?4.0f:20.0f;
            g.setColour(juce::Colour(0xff0b1422));g.fillRoundedRectangle(0,y,width,12,3);
            g.setColour(db>=0?juce::Colour(0xffff6b76):PhaseTwinTheme::accent());g.fillRoundedRectangle(0,y,width*juce::jlimit(0.0f,1.0f,(db+60)/66),12,3);
            g.setColour(juce::Colour(0xffe3eaf5));g.drawVerticalLine(int(width*60/66),y,y+12);
            g.setColour(juce::Colour(0xff0b1422).withAlpha(.8f));g.fillRoundedRectangle(3,y,105,12,2);g.setColour(juce::Colour(0xffe3eaf5));
            g.drawText(juce::String(ch==0?"L ":"R ")+juce::String(db,1)+" dBFS",6,int(y),99,12,juce::Justification::centredLeft);
        }
        auto badge=getLocalBounds().removeFromRight(49).reduced(1,3);g.setColour(clipped?juce::Colour(0xff823345):juce::Colour(0xff202c3c));g.fillRoundedRectangle(badge.toFloat(),5);
        g.setColour(clipped?juce::Colour(0xffffabb3):juce::Colour(0xffa2b1c7));g.drawText(clipped?"CLIP":"OUT",badge,juce::Justification::centred);if(hasKeyboardFocus(true)){g.setColour(PhaseTwinTheme::accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),4,1);}
    }
};

class PhaseTwinControls final : public juce::Component {
    PhaseTwinProcessor& owner;
    std::array<juce::TextButton,5> sections;
    std::array<int,5> sectionY{};
    unsigned expanded=0;
    const std::array<int,5> bodyHeights{210,65,82,65,55};
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<juce::Slider,6> sliders;
    std::array<juce::Label,6> labels;
    std::array<std::unique_ptr<SliderAttachment>,6> sliderAttachments;
    std::array<juce::ToggleButton,3> toggles;
    juce::Slider duckAmount,duckHarshness,grooveLimit,rotationFrequency,rotationQ;juce::ToggleButton rotation{"All-pass phase rotation"};juce::Label rotationFrequencyLabel,rotationQLabel;
    std::unique_ptr<SliderAttachment> rotationFrequencyAttachment,rotationQAttachment;std::unique_ptr<ButtonAttachment> rotationAttachment;
    std::array<juce::Slider,3> duckAdvancedSliders;std::array<juce::Label,3> duckAdvancedLabels;
    std::array<std::unique_ptr<SliderAttachment>,3> advancedAttachments;
    juce::ToggleButton advancedDuck{"Advanced attack / release"};std::unique_ptr<ButtonAttachment> advancedToggleAttachment;
    juce::ToggleButton groove{"Preserve kick/bass groove"},preview{"Preview before apply"};
    juce::ToggleButton verifyAfterApply{"Verify after apply"},multiSection{"Collect multiple sections"};
    std::unique_ptr<ButtonAttachment> verifyAttachment,multiAttachment;
    juce::TextButton evaluateSections{"Evaluate sections"},clearSections{"Clear sections"};juce::Label sectionsSummary;
    juce::ComboBox audition;
    juce::Label grooveLabel,auditionLabel;
    std::unique_ptr<SliderAttachment> grooveAttachment;
    std::unique_ptr<ButtonAttachment> grooveToggleAttachment,previewAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> auditionAttachment;
    juce::Label correctionLabel,duckAmountLabel,duckHarshnessLabel,duckMeter;
    juce::ToggleButton duckEnabled{"Sidechain ducking"};
    std::unique_ptr<SliderAttachment> duckAmountAttachment,duckHarshnessAttachment;
    std::unique_ptr<ButtonAttachment> duckEnabledAttachment;
    std::array<std::unique_ptr<ButtonAttachment>,3> buttonAttachments;
    juce::ComboBox profile,correctionMode;
    juce::TextButton kickPreset{"Kick + bass preset"};
    juce::Label profileLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> profileAttachment,correctionAttachment;
public:
    explicit PhaseTwinControls(PhaseTwinProcessor& p):owner(p){
        expanded=p.editorSections.load() & 31u;
        const char* sectionNames[]={"Analysis & groove","Ducking","Advanced ducking","Phase rotation","Audition"};
        for(int i=0;i<5;++i){addAndMakeVisible(sections[i]);sections[i].setName(sectionNames[i]);sections[i].setClickingTogglesState(true);sections[i].setToggleState((expanded & (1u<<i))!=0,juce::dontSendNotification);sections[i].setTooltip("Expand/collapse controls only. Hiding this section does not disable its processing.");sections[i].onClick=[this,i]{if(sections[i].getToggleState())expanded|=1u<<i;else expanded&=~(1u<<i);owner.editorSections=expanded;resized();if(onLayoutChanged)onLayoutChanged();};}
        const char* ids[]={"manual","range","confidence","band","mix","gain"};
        const char* names[]={"Timing trim (ms)","Search ± (ms)","Correlation gate · same source","Analysis focus (Hz)","Add B · 0 = target only","Output gain (dB)"};
        const char* tips[]={"Adds to the learned correction. Positive lag means A arrives later and is advanced relative to B.","Maximum timing search. This version supports ±20 ms.","Same-source analysis only: reject input windows below this correlation threshold. Kick/bass learning does not use this gate. This is not learn confidence.","Kick/bass measurement only: 25 Hz high-pass and this low-pass. It does not filter the output.","0: A only. 1: A plus B. B always feeds sidechain analysis, even at zero.","Applied equally to aligned and neutral comparison; reference sum can require headroom."};
        for(int i=0;i<6;++i){addAndMakeVisible(sliders[i]);addAndMakeVisible(labels[i]);labels[i].setText(names[i],juce::dontSendNotification);
            sliders[i].setName(names[i]);labels[i].setTooltip(tips[i]);
            sliders[i].setSliderStyle(juce::Slider::LinearHorizontal);sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,105,24);sliders[i].setTooltip(tips[i]);
            sliders[i].setTextValueSuffix(i<2?" ms":i==3?" Hz":i==5?" dB":"");
            sliders[i].setDoubleClickReturnValue(true,i==1?20:i==2?.65:i==3?180:0);
            sliderAttachments[i]=std::make_unique<SliderAttachment>(p.parameters,ids[i],sliders[i]);
            sliders[i].setNumDecimalPlacesToDisplay(i==0?3:i==3?0:i==2 || i==4?2:1);}
        const char* buttonIds[]={"auto","freeze","polarity"};
        const char* buttonNames[]={"Continuous tracking","Lock correction","Manual polarity flip"};
        for(int i=0;i<3;++i){addAndMakeVisible(toggles[i]);toggles[i].setButtonText(buttonNames[i]);buttonAttachments[i]=std::make_unique<ButtonAttachment>(p.parameters,buttonIds[i],toggles[i]);}
        toggles[0].setTooltip("Same-source mode only. Kick/bass uses session learning and holds its result.");
        addAndMakeVisible(correctionMode);addAndMakeVisible(correctionLabel);correctionLabel.setText("Automatic correction",juce::dontSendNotification);
        correctionMode.setName("Automatic correction");correctionMode.addItem("Timing + polarity",1);correctionMode.addItem("Preserve polarity",2);correctionMode.addItem("Preserve timing",3);
        correctionMode.setTooltip("Timing + polarity allows both automatic corrections. Preserve polarity changes timing only. Preserve timing changes polarity only and keeps existing timing, including learned advance and manual trim. Reset alignment first to restore original timing. Manual controls stay available.");
        correctionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"correctionMode",correctionMode);
        for(auto* c:{static_cast<juce::Component*>(&duckEnabled),static_cast<juce::Component*>(&duckAmount),static_cast<juce::Component*>(&duckHarshness),static_cast<juce::Component*>(&duckAmountLabel),static_cast<juce::Component*>(&duckHarshnessLabel),static_cast<juce::Component*>(&duckMeter)})addAndMakeVisible(c);
        duckEnabledAttachment=std::make_unique<ButtonAttachment>(p.parameters,"duckEnabled",duckEnabled);
        duckAmountLabel.setText("Ducking amount",juce::dontSendNotification);duckHarshnessLabel.setText("Ducking harshness",juce::dontSendNotification);
        for(auto* slider:{&duckAmount,&duckHarshness}){slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,75,24);slider->setDoubleClickReturnValue(true,50);slider->setTextValueSuffix(" %");}
        duckAmountAttachment=std::make_unique<SliderAttachment>(p.parameters,"duckAmount",duckAmount);duckHarshnessAttachment=std::make_unique<SliderAttachment>(p.parameters,"duckHarshness",duckHarshness);
        duckAmount.setTooltip("0% = no ducking. 100% = up to 24 dB reduction of A only. Actual reduction depends on B level. B is never ducked.");
        duckHarshness.setTooltip("Higher = quicker attack, shorter release and a firmer knee. Lower = rounder, slower ducking. Simple mode: attack 15–0.3 ms, release 250–60 ms. Advanced mode uses separate timing sliders; Harshness controls knee.");
        duckEnabled.setTooltip("Optional stereo-linked envelope ducking from latency-matched B. Applies after alignment, before output sum/gain. Analysis stays pre-ducking; After can show actual post-duck audio. Off by default.");
        addAndMakeVisible(profile);addAndMakeVisible(profileLabel);profileLabel.setText("Analysis profile",juce::dontSendNotification);
        profile.addItem("Same source / microphones",1);profile.addItem("Kick + bass / low-end focus",2);
        profileAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"profile",profile);
        for(auto* c:{static_cast<juce::Component*>(&groove),static_cast<juce::Component*>(&preview),static_cast<juce::Component*>(&grooveLimit),static_cast<juce::Component*>(&grooveLabel),static_cast<juce::Component*>(&audition),static_cast<juce::Component*>(&auditionLabel)})addAndMakeVisible(c);
        grooveToggleAttachment=std::make_unique<ButtonAttachment>(p.parameters,"preserveGroove",groove);previewAttachment=std::make_unique<ButtonAttachment>(p.parameters,"preview",preview);
        grooveLimit.setSliderStyle(juce::Slider::LinearHorizontal);grooveLimit.setTextBoxStyle(juce::Slider::TextBoxRight,false,75,24);grooveLimit.setTextValueSuffix(" ms");grooveLimit.setDoubleClickReturnValue(true,2);grooveAttachment=std::make_unique<SliderAttachment>(p.parameters,"grooveLimit",grooveLimit);
        grooveLabel.setText("Maximum new groove shift",juce::dontSendNotification);auditionLabel.setText("Audition correction",juce::dontSendNotification);
        audition.addItem("Timing + polarity",1);audition.addItem("Timing only",2);audition.addItem("Polarity only",3);audition.addItem("Neither",4);auditionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"audition",audition);
        groove.setTooltip("Kick/bass only. Limit new timing changes around the current offset and reject newly quiet 2 ms bins across detected kick intervals. Low-band energy heuristic, not a guarantee of musical continuity. Reset removes an existing large advance.");
        preview.setTooltip("Default On: Analyze proposes a correction; Apply commits it. Off applies reliable results immediately. Preview is not saved as a pending recommendation.");audition.setTooltip("Hear stored timing, polarity, both or neither at matched latency. Manual trim/flip are included. Hear unaligned overrides this selector; release it to hear this choice. Ducking remains active.");
        for(auto* c:{static_cast<juce::Component*>(&verifyAfterApply),static_cast<juce::Component*>(&multiSection),static_cast<juce::Component*>(&evaluateSections),static_cast<juce::Component*>(&clearSections),static_cast<juce::Component*>(&sectionsSummary)})addAndMakeVisible(c);
        verifyAttachment=std::make_unique<ButtonAttachment>(p.parameters,"verifyAfterApply",verifyAfterApply);multiAttachment=std::make_unique<ButtonAttachment>(p.parameters,"multiSection",multiSection);
        verifyAfterApply.setTooltip("Default On: after Apply or immediate learning, compare fresh normal corrected audio against unaligned A at matched latency. Kick: four seconds of low-band interaction; same source: fresh correlation windows. Includes current phase rotation, before ducking/mix/gain. Never adjusts the correction. Normal audition is required; stopped playback waits.");
        multiSection.setTooltip("Optional: Analyze collects one passage without applying. Move playback to another representative passage, collect 2–4 sections, then Evaluate sections. A common candidate must help at least 75% and harm none by more than 0.02. Audio is held in memory only, not saved with the session. Settings changes, Cancel, Reset, Undo/Redo or Apply clear the collection.");
        evaluateSections.onClick=[&p]{p.evaluateSectionsRequest.fetch_add(1);};clearSections.onClick=[&p]{p.clearSectionsRequest.fetch_add(1);};
        clearSections.setTooltip("Discard the collected analysis audio, not the held correction or Undo history.");sectionsSummary.setTooltip("Per-section changes in normalized interaction/correlation for the common proposal. Conservative validation uses the captured passages, not independent future audio. Use post-Apply verification for fresh material.");
        addAndMakeVisible(advancedDuck);advancedToggleAttachment=std::make_unique<ButtonAttachment>(p.parameters,"duckAdvanced",advancedDuck);
        const char* advancedIds[]={"duckAttack","duckRelease","duckSensitivity"};const char* advancedNames[]={"Ducking attack","Ducking release","Detector sensitivity"};const double advancedDefaults[]={3,125,0};
        for(int i=0;i<3;++i){addAndMakeVisible(duckAdvancedSliders[i]);addAndMakeVisible(duckAdvancedLabels[i]);duckAdvancedLabels[i].setText(advancedNames[i],juce::dontSendNotification);auto& slider=duckAdvancedSliders[i];slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,85,24);slider.setTextValueSuffix(i==2?" dB":" ms");slider.setDoubleClickReturnValue(true,advancedDefaults[i]);advancedAttachments[i]=std::make_unique<SliderAttachment>(p.parameters,advancedIds[i],slider);}
        advancedDuck.setTooltip("Off: Harshness controls attack/release/knee. On: explicit attack and release take over; Harshness still controls knee. Sensitivity applies in either mode.");
        duckAdvancedSliders[0].setTooltip("Advanced attack 0.1–100 ms. Actual gain is also smoothed over 1 ms.");duckAdvancedSliders[1].setTooltip("Advanced release 10–1000 ms.");duckAdvancedSliders[2].setTooltip("Detector-only gain: +dB reacts more to quieter B; -dB reacts less. Does not boost B in the output. Smoothed over 5 ms.");
        for(auto* c:{static_cast<juce::Component*>(&rotation),static_cast<juce::Component*>(&rotationFrequency),static_cast<juce::Component*>(&rotationQ),static_cast<juce::Component*>(&rotationFrequencyLabel),static_cast<juce::Component*>(&rotationQLabel)})addAndMakeVisible(c);
        rotationAttachment=std::make_unique<ButtonAttachment>(p.parameters,"rotation",rotation);rotationFrequencyLabel.setText("All-pass centre",juce::dontSendNotification);rotationQLabel.setText("All-pass Q",juce::dontSendNotification);
        for(auto* slider:{&rotationFrequency,&rotationQ}){slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,85,24);}
        rotationFrequency.setTextValueSuffix(" Hz");rotationFrequency.setDoubleClickReturnValue(true,80);rotationQ.setDoubleClickReturnValue(true,.707);rotationFrequencyAttachment=std::make_unique<SliderAttachment>(p.parameters,"rotationFrequency",rotationFrequency);rotationQAttachment=std::make_unique<SliderAttachment>(p.parameters,"rotationQ",rotationQ);
        rotation.setTooltip("Optional second-order all-pass on A only. Unity steady-state magnitude; frequency-dependent phase and group delay, NOT a constant degree shift. Off by default. Manual only; Analyze does not optimize rotation. Neutral and timing-only audition bypass it.");
        profile.onChange=[this]{sliders[3].setEnabled(profile.getSelectedId()==2);sliders[2].setEnabled(profile.getSelectedId()==1);toggles[0].setEnabled(profile.getSelectedId()==1);groove.setEnabled(profile.getSelectedId()==2);grooveLimit.setEnabled(profile.getSelectedId()==2);};
        addAndMakeVisible(kickPreset);kickPreset.setTooltip("Set kick profile and 180 Hz analysis focus; preserve the automatic correction mode. Existing correction is held until you learn. The same-source correlation gate and output gain are left unchanged.");
        kickPreset.onClick=[&p]{
            p.beginHistoryAction();
            auto set=[&p](const char* id,float value){auto* parameter=p.parameters.getParameter(id);parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(value));parameter->endChangeGesture();};
            set("profile",1);set("band",180);set("auto",0);set("freeze",0);p.endHistoryAction();
        };
        for(auto& slider:sliders)slider.setScrollWheelEnabled(false);
        for(auto& slider:duckAdvancedSliders)slider.setScrollWheelEnabled(false);
        for(auto* slider:{&duckAmount,&duckHarshness,&grooveLimit,&rotationFrequency,&rotationQ})slider->setScrollWheelEnabled(false);
        sliders[3].setEnabled(profile.getSelectedId()==2);sliders[2].setEnabled(profile.getSelectedId()==1);toggles[0].setEnabled(profile.getSelectedId()==1);groove.setEnabled(profile.getSelectedId()==2);grooveLimit.setEnabled(profile.getSelectedId()==2);
    }
    std::function<void()> onLayoutChanged;
    int preferredHeight()const{int height=134+5*32+8;for(int i=0;i<5;++i)if(expanded & (1u<<i))height+=bodyHeights[i];return height;}
    void refreshSections(){const unsigned saved=owner.editorSections.load() & 31u;if(saved!=expanded){expanded=saved;for(int i=0;i<5;++i)sections[i].setToggleState((saved & (1u<<i))!=0,juce::dontSendNotification);resized();if(onLayoutChanged)onLayoutChanged();}}
    void resized()override{
        const int column=getWidth()/3;
        auto field=[&](juce::Slider& slider,juce::Label& label,int col,int y){label.setBounds(col*column+10,y,column-20,18);slider.setBounds(col*column+10,y+19,column-20,27);};
        profileLabel.setBounds(10,8,125,26);profile.setBounds(140,8,310,28);kickPreset.setBounds(465,8,185,28);
        correctionLabel.setBounds(10,40,145,26);correctionMode.setBounds(155,40,205,26);preview.setBounds(375,40,190,26);toggles[2].setBounds(570,40,175,26);toggles[1].setBounds(750,40,std::max(100,getWidth()-760),26);
        field(sliders[0],labels[0],0,78);field(sliders[4],labels[4],1,78);field(sliders[5],labels[5],2,78);
        int y=134;const char* names[]={"Analysis & groove","Ducking","Advanced ducking","Phase rotation","Audition"};
        for(int i=0;i<5;++i){const bool open=(expanded & (1u<<i))!=0;sectionY[i]=y;sections[i].setBounds(8,y,getWidth()-16,28);juce::String name=(open?"−  ":"+  ")+juce::String(names[i]);
            if(i==0 && owner.parameters.getRawParameterValue("multiSection")->load()>.5f)name+="  ·  SECTIONS "+juce::String(owner.sectionCount.load())+" / 4";
            if(i==0 && owner.parameters.getRawParameterValue("profile")->load()>.5f)name+=owner.parameters.getRawParameterValue("preserveGroove")->load()>.5f?"  ·  GROOVE ON":"  ·  FREE TIMING";
            if(i==1)name+=owner.parameters.getRawParameterValue("duckEnabled")->load()>.5f?"  ·  ON":"  ·  OFF";
            if(i==2)name+=owner.parameters.getRawParameterValue("duckAdvanced")->load()>.5f?"  ·  ON":"  ·  SIMPLE MODE";
            if(i==3)name+=owner.parameters.getRawParameterValue("rotation")->load()>.5f?"  ·  ON":"  ·  OFF";
            sections[i].setButtonText(name);y+=32;const int top=y;
            auto show=[&](std::initializer_list<juce::Component*> controls){for(auto* control:controls)control->setVisible(open);};
            if(i==0){show({&sliders[1],&labels[1],&sliders[2],&labels[2],&sliders[3],&labels[3],&groove,&grooveLimit,&grooveLabel,&toggles[0],&verifyAfterApply,&multiSection,&evaluateSections,&clearSections,&sectionsSummary});field(sliders[1],labels[1],0,top);field(sliders[2],labels[2],1,top);field(sliders[3],labels[3],2,top);groove.setBounds(10,top+53,column-20,26);field(grooveLimit,grooveLabel,1,top+51);toggles[0].setBounds(column*2+10,top+54,column-20,26);verifyAfterApply.setBounds(10,top+107,column-20,26);multiSection.setBounds(column+10,top+107,column-20,26);evaluateSections.setBounds(column*2+10,top+107,std::max(110,(column-30)/2),26);clearSections.setBounds(column*2+20+(column-30)/2,top+107,std::max(100,(column-30)/2),26);
                const int count=owner.sectionCount.load();const bool collect=owner.parameters.getRawParameterValue("multiSection")->load()>.5f;evaluateSections.setEnabled(collect && count>=2 && owner.learnState.load()!=1 && owner.learnState.load()!=10);clearSections.setEnabled(collect && (count>0 || owner.learnState.load()==1));
                juce::String summary=collect?"Collected "+juce::String(count)+" / 4 · move to another passage, then Analyze":"Single-section analysis · optional collection: 2–4 passages";
                if(collect && owner.recommendationReady.load()){summary+="\nSection improvements:";for(int s=0;s<count;++s)summary+="  "+juce::String(s+1)+": "+juce::String(owner.sectionGains[s].load(),3);}
                sectionsSummary.setText(summary,juce::dontSendNotification);sectionsSummary.setBounds(10,top+140,getWidth()-20,55);
            }
            if(i==1){show({&duckEnabled,&duckAmount,&duckHarshness,&duckAmountLabel,&duckHarshnessLabel,&duckMeter});duckEnabled.setBounds(10,top,column-20,26);duckMeter.setBounds(10,top+28,column-20,22);field(duckAmount,duckAmountLabel,1,top);field(duckHarshness,duckHarshnessLabel,2,top);}
            if(i==2){show({&advancedDuck});advancedDuck.setBounds(10,top,getWidth()-20,26);for(int k=0;k<3;++k){duckAdvancedSliders[k].setVisible(open);duckAdvancedLabels[k].setVisible(open);field(duckAdvancedSliders[k],duckAdvancedLabels[k],k,top+28);}}
            if(i==3){show({&rotation,&rotationFrequency,&rotationQ,&rotationFrequencyLabel,&rotationQLabel});rotation.setBounds(10,top,column-20,26);field(rotationFrequency,rotationFrequencyLabel,1,top);field(rotationQ,rotationQLabel,2,top);}
            if(i==4){show({&audition,&auditionLabel});auditionLabel.setBounds(10,top,145,26);audition.setBounds(165,top,250,28);}
            if(open)y+=bodyHeights[i];
        }
    }
    void setRotationEnabled(bool enabled){rotationFrequency.setEnabled(enabled);rotationQ.setEnabled(enabled);}
    void setAdvancedEnabled(bool enabled){duckAdvancedSliders[0].setEnabled(enabled);duckAdvancedSliders[1].setEnabled(enabled);}
    void setDuckReduction(float db){duckMeter.setText("Reduction: "+juce::String(db,1)+" dB",juce::dontSendNotification);}
    void paint(juce::Graphics& g)override{
        auto bounds=getLocalBounds().toFloat().reduced(.5f);g.setColour(juce::Colour(0xff172233));g.fillRoundedRectangle(bounds,10);
        g.setColour(juce::Colour(0xff30425b));g.drawRoundedRectangle(bounds,10,1);
        g.setColour(juce::Colour(0xff2a3b52));g.drawHorizontalLine(129,12,float(getWidth()-12));
    }
};

class PhaseTwinEditor final : public juce::AudioProcessorEditor,private juce::Timer {
    PhaseTwinTheme theme;
    PhaseTwinProcessor& processor;
    PhaseTwinControls controls;
    juce::Viewport controlsViewport;
    PhaseTwinOutputMeter outputMeter;
    PhaseTwinSpectrum spectrum;juce::TextButton spectrumView{"Spectrum"};
    PhaseTwinScope oscilloscope;
    juce::Label title,status,hint,legend;
    juce::ComboBox channel,beatLength;
    juce::TextButton scopeMode{"Capture: Trigger"},scopeStyle{"Waveforms: Filled"};
    juce::TextButton scopeComposition{"Scope: Stacked"},duckView{"After: Pre-duck"};
    juce::ToggleButton visibleA{"Show A"},visibleB{"Show B"};
    juce::Label viewHint;
    juce::TextButton hearProposal{"Hear proposal"};
    juce::TextButton applyRecommendation{"Apply recommendation"},undoLearn{"Undo"},redoAudio{"Redo"},help{"Quick help"};
    std::uint32_t seenViewRevision=0;
    juce::TooltipWindow tooltips{this,500};
    juce::Slider windowMs;
    juce::ToggleButton tempoSync{"DAW sync"};
    juce::TextButton alignNow{"ANALYZE · 2 SECONDS"},resetAlign{"Reset alignment"};
    juce::ToggleButton compare{"Hear unaligned"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compareAttachment;
    juce::Label score,learnConfidence;
    unsigned lastBlocks=0;
    int idleTicks=0;
    juce::ToggleButton pause{"Hold view"};
public:
    explicit PhaseTwinEditor(PhaseTwinProcessor& p):AudioProcessorEditor(p),processor(p),controls(p),outputMeter(p),spectrum(p){
        setLookAndFeel(&theme);
        alignNow.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff28544b));
        alignNow.setColour(juce::TextButton::textColourOffId,juce::Colour(0xffdffff3));
        score.setColour(juce::Label::textColourId,PhaseTwinTheme::accent());
        learnConfidence.setColour(juce::Label::textColourId,juce::Colour(0xffb5a5ef));
        hint.setColour(juce::Label::textColourId,juce::Colour(0xff8fa1ba));
        for(auto* c : {static_cast<juce::Component*>(&outputMeter),static_cast<juce::Component*>(&spectrum),static_cast<juce::Component*>(&spectrumView),static_cast<juce::Component*>(&title),static_cast<juce::Component*>(&status),static_cast<juce::Component*>(&hint),static_cast<juce::Component*>(&legend),static_cast<juce::Component*>(&controlsViewport),static_cast<juce::Component*>(&oscilloscope),static_cast<juce::Component*>(&channel),static_cast<juce::Component*>(&beatLength),static_cast<juce::Component*>(&windowMs),static_cast<juce::Component*>(&tempoSync),static_cast<juce::Component*>(&alignNow),static_cast<juce::Component*>(&resetAlign),static_cast<juce::Component*>(&compare),static_cast<juce::Component*>(&score),static_cast<juce::Component*>(&learnConfidence),static_cast<juce::Component*>(&pause),static_cast<juce::Component*>(&scopeMode),static_cast<juce::Component*>(&scopeStyle),static_cast<juce::Component*>(&scopeComposition),static_cast<juce::Component*>(&visibleA),static_cast<juce::Component*>(&visibleB),static_cast<juce::Component*>(&viewHint),static_cast<juce::Component*>(&duckView),static_cast<juce::Component*>(&applyRecommendation),static_cast<juce::Component*>(&hearProposal),static_cast<juce::Component*>(&undoLearn),static_cast<juce::Component*>(&redoAudio),static_cast<juce::Component*>(&help)})addAndMakeVisible(c);
        title.setText("PHASETWIN",juce::dontSendNotification);
        status.setFont(juce::Font(juce::FontOptions(13.0f)));
        status.setJustificationType(juce::Justification::centredLeft);
        title.setFont(juce::Font(juce::FontOptions(24.0f)));
        title.setColour(juce::Label::textColourId,juce::Colour(0xff72e9c6));
        legend.setText("A = mint     B = amber",juce::dontSendNotification);
        hint.setText("1. Target on A · reference into sidechain B.  2. Analyze · inspect · Apply.  3. Compare with Hear unaligned.\nAdd B = 0: target only. Positive offset advances A. Preserve timing keeps offsets; Reset clears them.",juce::dontSendNotification);
        hint.setFont(juce::Font(juce::FontOptions(12.0f)));
        viewHint.setFont(juce::Font(juce::FontOptions(12.0f)));
        hint.setColour(juce::Label::textColourId,juce::Colour(0xffb5bcc9));
        channel.addItem("Left",1);channel.addItem("Right",2);channel.addItem("Mono average",3);channel.setSelectedId(1);
        scopeStyle.setClickingTogglesState(true);scopeStyle.setToggleState(true,juce::dontSendNotification);scopeStyle.onClick=[this]{applyScopeButtons();saveView();};
        scopeStyle.setName("Waveform style");scopeMode.setName("Scope capture mode");channel.setName("Scope channel");beatLength.setName("Scope note division");windowMs.setName("Scope window milliseconds");oscilloscope.setName("Before and after waveform comparison");
        scopeComposition.setClickingTogglesState(true);scopeComposition.setName("Switch stacked/summed scope");scopeComposition.setTooltip("Click to switch both Before/After panels between source traces and unity sum. Display only; does not change output mixing.");
        duckView.setClickingTogglesState(true);duckView.setName("After waveform pre/post ducking");duckView.setTooltip("Switch After between aligned A before ducking and actual ducked A. Summed adds unducked B. Scale is shared across both choices; analysis scores remain pre-ducking. Reduction history follows the same captured window and Hold.");
        duckView.onClick=[this]{applyDuckView();saveView();};
        scopeComposition.onClick=[this]{applyComposition();saveView();};
        scopeStyle.setTooltip("Click to switch Filled/Lines. Lines show waveform outlines; filled adds translucent signed areas while retaining outlines and peaks. Neither changes the audio or the shared scale.");
        visibleA.setToggleState(true,juce::dontSendNotification);visibleB.setToggleState(true,juce::dontSendNotification);
        visibleA.onClick=visibleB.onClick=[this]{oscilloscope.setVisibleWaves(visibleA.getToggleState(),visibleB.getToggleState());saveView();};
        visibleA.setTooltip("Hide/show target A visually. Audio and analysis continue.");visibleB.setTooltip("Hide/show reference B visually. Triggering, audio and analysis continue.");
        viewHint.setText("Display only",juce::dontSendNotification);
        scopeMode.setClickingTogglesState(true);scopeMode.setToggleState(true,juce::dontSendNotification);
        scopeMode.onClick=[this]{applyScopeButtons();saveView();};
        scopeMode.setTooltip("Click to switch Reference trigger/Rolling. Reference trigger holds the latest complete window, anchored to B. Rolling scrolls continuously. Hold freezes either view. This does not change audio or learning.");
        channel.onChange=[this]{oscilloscope.setChannel(channel.getSelectedId()-1);saveView();};
        beatLength.addItem("1/64 note",1);beatLength.addItem("1/32 note",2);beatLength.addItem("1/16 note",3);
        beatLength.addItem("1/8 note",4);beatLength.addItem("1/4 note",5);beatLength.addItem("1/2 note",6);beatLength.addItem("1/1 note",7);beatLength.setSelectedId(5);beatLength.onChange=[this]{saveView();};
        windowMs.setSliderStyle(juce::Slider::LinearHorizontal);windowMs.setRange(2,4000,1);windowMs.setSkewFactorFromMidPoint(125);
        windowMs.setTextBoxStyle(juce::Slider::TextBoxRight,false,80,26);windowMs.setTextValueSuffix(" ms");windowMs.setValue(125);windowMs.onValueChange=[this]{saveView();};
        tempoSync.setToggleState(true,juce::dontSendNotification);windowMs.setEnabled(false);
        tempoSync.onClick=[this]{windowMs.setEnabled(!tempoSync.getToggleState());beatLength.setEnabled(tempoSync.getToggleState());saveView();};
        alignNow.setTooltip("Capture four seconds in kick mode or two seconds in same-source mode while both sources play. Apply stable timing/polarity according to Automatic correction, then hold. Manual overrides are preserved. Starts a fresh capture, unlocks correction, disables tracking and exits unaligned audition.");
        alignNow.onClick=[this]{
            if(processor.learnState.load()==1){if(auto* parameter=processor.parameters.getParameter("auto"))parameter->setValueNotifyingHost(0);processor.cancelRequest.fetch_add(1);return;}
            auto set=[this](const char* id,float normalized){if(auto* parameter=processor.parameters.getParameter(id)){parameter->beginChangeGesture();parameter->setValueNotifyingHost(normalized);parameter->endChangeGesture();}};
            processor.beginHistoryAction();set("auto",0);set("freeze",0);set("compare",0);processor.endHistoryAction();
            processor.alignRequest.fetch_add(1);
        };
        compareAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"compare",compare);
        compare.setTooltip("Hear neutral timing/polarity at the same latency and output level. Learned values stay intact.");
        resetAlign.onClick=[this]{
            processor.beginHistoryAction();
            auto* trim=processor.parameters.getParameter("manual");trim->beginChangeGesture();trim->setValueNotifyingHost(trim->convertTo0to1(0));trim->endChangeGesture();
            for(auto* id:{"polarity","auto","compare"}){auto* parameter=processor.parameters.getParameter(id);parameter->beginChangeGesture();parameter->setValueNotifyingHost(0);parameter->endChangeGesture();}
            processor.resetRequest.fetch_add(1);processor.endHistoryAction();
        };
        score.setFont(juce::Font(juce::FontOptions(17)));learnConfidence.setFont(juce::Font(juce::FontOptions(17)));
        score.setTooltip("Kick mode: session interaction score; 50 is neutral, below 50 indicates cancellation. Same source: positive correlation × 100. Confidence is heuristic, not a probability.");
        learnConfidence.setTooltip("Kick mode: support and agreement across detected hits, including hits excluded from fitting. Same source: peak quality and stable window consensus. A heuristic, not a success probability.");
        pause.setTooltip("Freeze the visual only. Audio and learning continue. Resume keeps the last frame until a fresh complete capture is available.");
        beatLength.setTooltip("Whole-note fractions: 1/4 is one quarter-note beat; 1/1 is four quarter-note beats. Duration follows DAW BPM, with a 4-second maximum. No bar or time-signature synchronisation is implied.");
        tempoSync.setTooltip("Use the DAW tempo for scope length. If tempo is unavailable, use 120 BPM. Turn off to type a window in milliseconds.");
        windowMs.setTooltip("Scope window in milliseconds; independent of delay search. Editable when DAW sync is off; capped at 4000 ms.");
        pause.onClick=[this]{oscilloscope.setPaused(pause.getToggleState());spectrum.setPaused(pause.getToggleState());};
        undoLearn.setTooltip("Undo audio settings or applied alignment (up to 128 steps). A slider drag is one step. Display and temporary proposal audition are excluded. Loaded sessions start a fresh history.");
        undoLearn.onClick=[this]{processor.undoAudio();};
        redoAudio.setTooltip("Restore the next undone audio state. A new audio edit discards the redo branch.");redoAudio.onClick=[this]{processor.redoAudio();};
        help.onClick=[] {juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,"PhaseTwin · quick help","1. Put PhaseTwin on target A and route reference B into the external sidechain. Both meters must move.\n2. Choose Same source for related microphones, or Kick + bass for low-end interaction. Play a representative section and Analyze. Preview is on by default: inspect the proposal and press Apply.\n3. Read confidence and compare with Hear unaligned. Automatic correction selects both changes, timing only, or polarity only. Preserve timing holds existing offsets; Reset restores original timing. Cancel holds your previous correction; Undo/Redo moves through audio settings and applied alignments (128 steps). Display and Hear proposal are excluded. Session reload starts a fresh history.\n\nPositive timing advances A relative to B; negative delays A. Trim adds to learned timing, within ±20 ms. The DAW receives reported compensation latency.\n\nScope Lines/Filled, Stacked/Summed, Show A/B and Hold change the display only. Summed means unity A+B, independent of output mix/gain. Reference trigger holds complete captures. Fill is not gain.\n\nKick score is a session candidate measurement, not live output verification. Confidence is heuristic. Preserve polarity holds current polarity; Preserve timing holds current timing. Optional ducking reduces A from B after alignment. Amount sets up to 24 dB depth; Harshness makes attack/release/knee faster and firmer. Alignment metrics stay pre-ducking. After: Pre/Post-duck switches the waveform; the reduction graph shares its capture and Hold. Spectral correction and multi-instance grouping are not yet available.");};
        hearProposal.setClickingTogglesState(true);hearProposal.setTooltip("Temporarily hear the proposal without changing held correction or Undo. Full timing/polarity overrides component audition and Hear unaligned; their settings are preserved. Current trim, manual flip, rotation, ducking and output gain stay as configured. After displays show heard audio; release Hold to refresh captures. Apply commits; new analysis/settings or invalidation ends audition.");hearProposal.onClick=[this]{processor.hearProposalRequested=hearProposal.getToggleState();};
        applyRecommendation.onClick=[this]{processor.captureHistory();processor.applyRequest.fetch_add(1);};applyRecommendation.setTooltip("Apply the currently displayed recommendation on the next audio callback. Changes to analysis settings invalidate the proposal. Previous correction is saved for Undo.");
        spectrumView.setTooltip("Wrapped phase difference from 64 logarithmic FFT bands (20 Hz–20 kHz), strongest analysis channel. 0° = in phase; ±180° = opposing. Band brightness reflects within-band phase concentration, not calibrated coherence/confidence. Low-frequency resolution depends on sample rate. No auto spectral correction.");spectrumView.setClickingTogglesState(true);spectrumView.onClick=[this]{const bool spectral=spectrumView.getToggleState();spectrum.setVisible(spectral);oscilloscope.setVisible(!spectral);spectrumView.setButtonText(spectral?"Waveforms":"Spectrum");};spectrum.setVisible(false);
        controlsViewport.setName("Collapsible controls; scroll for more sections");controlsViewport.setViewedComponent(&controls,false);controlsViewport.setScrollBarsShown(true,false);controls.onLayoutChanged=[this]{resized();};
        restoreView();setResizable(true,true);setResizeLimits(940,900,1600,1400);setSize(1080,960);startTimerHz(30);
    }
    ~PhaseTwinEditor()override{stopTimer();setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g)override{
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff172235),0,0,juce::Colour(0xff0b121d),0,float(getHeight()),false));g.fillRect(getLocalBounds());
        g.setColour(juce::Colour(0xff30425b));g.drawHorizontalLine(85,20,float(getWidth()-20));
        g.setColour(juce::Colour(0xff101a28));g.fillRoundedRectangle(20,89,float(getWidth()-40),59,8);
        g.setColour(juce::Colour(0xff293b52));g.drawHorizontalLine(getHeight()-79,20,float(getWidth()-20));
        g.setColour(PhaseTwinTheme::accent());g.fillRoundedRectangle(5,17,3,26,1.5f);
    }
    void resized()override{
        title.setBounds(20,10,270,36);hearProposal.setBounds(300,12,std::clamp(getWidth()-835,110,180),34);outputMeter.setBounds(getWidth()-525,12,250,34);alignNow.setBounds(getWidth()-255,12,235,34);status.setBounds(20,88,getWidth()-40,60);
        score.setBounds(20,50,390,32);learnConfidence.setBounds(420,50,getWidth()-615,32);compare.setBounds(getWidth()-185,51,165,30);
        legend.setBounds(20,156,170,26);channel.setBounds(200,156,100,26);tempoSync.setBounds(310,156,105,26);
        beatLength.setBounds(420,156,115,26);windowMs.setBounds(545,156,std::min(300,getWidth()-565),26);
        scopeStyle.setBounds(20,188,160,26);scopeComposition.setBounds(190,188,155,26);visibleA.setBounds(355,188,75,26);visibleB.setBounds(435,188,75,26);scopeMode.setBounds(520,188,155,26);pause.setBounds(685,188,100,26);viewHint.setBounds(0,0,0,0);spectrumView.setBounds(795,188,getWidth()-815,26);
        const int controlsHeight=std::min(controls.preferredHeight(),std::max(200,getHeight()-224-310-95));
        controlsViewport.setBounds(20,getHeight()-95-controlsHeight,getWidth()-40,controlsHeight);controls.setSize(controlsViewport.getWidth()-controlsViewport.getScrollBarThickness(),controls.preferredHeight());
        oscilloscope.setBounds(20,224,getWidth()-40,controlsViewport.getY()-236);spectrum.setBounds(oscilloscope.getBounds());
        applyRecommendation.setBounds(605,getHeight()-72,165,28);duckView.setBounds(420,getHeight()-72,175,28);resetAlign.setBounds(20,getHeight()-72,150,28);undoLearn.setBounds(180,getHeight()-72,110,28);redoAudio.setBounds(300,getHeight()-72,110,28);help.setBounds(getWidth()-150,getHeight()-72,130,28);hint.setBounds(20,getHeight()-38,getWidth()-40,34);
    }
    void applyComposition(){const bool sum=scopeComposition.getToggleState();scopeComposition.setButtonText(sum?"Scope: Summed":"Scope: Stacked");oscilloscope.setSummed(sum);visibleA.setEnabled(!sum);visibleB.setEnabled(!sum);legend.setText(sum?"SUM A+B = violet":"A = mint     B = amber",juce::dontSendNotification);viewHint.setTooltip(sum?"Unity sum for comparison only. Add B and output gain do not affect this view. Both panels share amplitude scale.":"Both source traces share amplitude scale. Display controls do not affect audio.");}
    void applyDuckView(){const bool post=duckView.getToggleState();duckView.setButtonText(post?"After: Post-duck":"After: Pre-duck");oscilloscope.setPostDuck(post);}
    void applyScopeButtons(){
        const bool filled=scopeStyle.getToggleState(),triggered=scopeMode.getToggleState();
        scopeStyle.setButtonText(filled?"Waveforms: Filled":"Waveforms: Lines");
        scopeMode.setButtonText(triggered?"Reference trigger":"Capture: Rolling");
        oscilloscope.setFilled(filled);oscilloscope.setTriggered(triggered);
    }
    void saveView(){phasetwin::ScopePreferences p;p.style=scopeStyle.getToggleState()?1:0;p.summed=scopeComposition.getToggleState();p.postDuck=duckView.getToggleState();p.mode=scopeMode.getToggleState()?0:1;p.channel=channel.getSelectedId()-1;p.division=beatLength.getSelectedId();p.sync=tempoSync.getToggleState();p.milliseconds=windowMs.getValue();p.showA=visibleA.getToggleState();p.showB=visibleB.getToggleState();processor.scopePreferences=p.packed();}
    void restoreView(){const auto p=phasetwin::ScopePreferences::unpack(processor.scopePreferences.load());seenViewRevision=processor.scopeViewRevision.load();duckView.setToggleState(p.postDuck,juce::dontSendNotification);applyDuckView();scopeComposition.setToggleState(p.summed,juce::dontSendNotification);scopeStyle.setToggleState(p.style==1,juce::dontSendNotification);scopeMode.setToggleState(p.mode==0,juce::dontSendNotification);channel.setSelectedId(p.channel+1,juce::dontSendNotification);beatLength.setSelectedId(p.division,juce::dontSendNotification);tempoSync.setToggleState(p.sync,juce::dontSendNotification);windowMs.setValue(p.milliseconds,juce::dontSendNotification);visibleA.setToggleState(p.showA,juce::dontSendNotification);visibleB.setToggleState(p.showB,juce::dontSendNotification);windowMs.setEnabled(!p.sync);beatLength.setEnabled(p.sync);applyScopeButtons();oscilloscope.setChannel(p.channel);oscilloscope.setVisibleWaves(p.showA,p.showB);applyComposition();}
    void timerCallback()override{
        controls.refreshSections();controls.resized();
        spectrum.update();controls.setRotationEnabled(processor.parameters.getRawParameterValue("rotation")->load()>.5f);
        outputMeter.update(processor.outputPeakL.load(),processor.outputPeakR.load(),processor.outputClipped.load());controls.setAdvancedEnabled(processor.parameters.getRawParameterValue("duckAdvanced")->load()>.5f);
        controls.setDuckReduction(processor.duckReductionDb.load());
        const bool duckingOn=processor.parameters.getRawParameterValue("duckEnabled")->load()>0.5f;duckView.setEnabled(duckingOn);oscilloscope.setDuckingEnabled(duckingOn);
        if(seenViewRevision!=processor.scopeViewRevision.load())restoreView();
        undoLearn.setEnabled(processor.undoAvailable.load() && processor.historyNavigationReady());redoAudio.setEnabled(processor.redoAvailable.load() && processor.historyNavigationReady());undoLearn.setButtonText("Undo ("+juce::String(processor.undoSteps.load())+")");redoAudio.setButtonText("Redo ("+juce::String(processor.redoSteps.load())+")");
        const double beats=phasetwin::scopeDivisionBeats(beatLength.getSelectedId());
        const bool sync=tempoSync.getToggleState(),gotBpm=processor.bpmAvailable.load();
        const double bpm=gotBpm?processor.hostBpm.load():120.0;
        const double duration=phasetwin::scopeDurationMs(sync,windowMs.getValue(),beats,bpm,gotBpm);
        if(sync)windowMs.setValue(duration,juce::dontSendNotification);
        oscilloscope.setWindow(duration,sync?(gotBpm?"DAW ":"fallback ")+juce::String(bpm,1)+" BPM":"manual");
        phasetwin::ScopePacket packet;
        // Apply the requested window before consuming new audio.
        for(int i=0;i<64 && processor.scope.pop(packet);++i)oscilloscope.ingest(packet);
        const auto blocks=processor.audioBlocks.load();idleTicks=blocks==lastBlocks?idleTicks+1:0;lastBlocks=blocks;
        if(idleTicks>20)oscilloscope.clearIfLive();
        oscilloscope.repaint();
        const int state=processor.learnState.load();const bool canAudition=processor.recommendationReady.load() && processor.parameters.getRawParameterValue("freeze")->load()<.5f && processor.referencePresent.load();const bool hearing=processor.hearingProposal.load();applyRecommendation.setEnabled(canAudition);hearProposal.setEnabled(canAudition);hearProposal.setToggleState(processor.hearProposalRequested.load() && canAudition,juce::dontSendNotification);compare.setEnabled(!hearing);
        const bool collect=processor.parameters.getRawParameterValue("multiSection")->load()>.5f;
        alignNow.setEnabled(state!=10 && (!collect || state==1 || processor.sectionCount.load()<phasetwin::maxSections));
        alignNow.setButtonText(state==1?"CANCEL · "+juce::String(processor.learnProgress.load()*100,0)+"%":(collect?(processor.sectionCount.load()>=phasetwin::maxSections?juce::String("SECTIONS FULL · EVALUATE"):"COLLECT SECTION "+juce::String(processor.sectionCount.load()+1)):(processor.parameters.getRawParameterValue("profile")->load()>0.5f?juce::String("LEARN · 4 SECONDS"):juce::String("ANALYZE · 2 SECONDS"))));
        score.setText((state==0 || state==5 || state==6) && processor.parameters.getRawParameterValue("profile")->load()>0.5f?juce::String("Kick score  —  learn to measure"):idleTicks>20?juce::String("Alignment score  —  (no live processing)"):(processor.parameters.getRawParameterValue("profile")->load()>0.5f?juce::String("Kick candidate  "):juce::String("After score  "))+juce::String(processor.alignmentScore.load(),1)+"%  (before "+juce::String(processor.beforeScore.load(),1)+"%)",juce::dontSendNotification);
        if(collect && state!=7 && state!=2)score.setText("Sections "+juce::String(processor.sectionCount.load())+" / 4 · Evaluate after collecting 2–4 passages",juce::dontSendNotification);
        learnConfidence.setText((state==0 || state==5 || state==6 || state==9 || state==10)?juce::String("Confidence  —"):"Confidence  "+juce::String(processor.reliability.load()*100,0)+"%",juce::dontSendNotification);
        juce::String text;
        const auto reason=static_cast<phasetwin::AnalysisReason>(processor.analysisReason.load());
        if(idleTicks>20)text="Waiting for DAW audio processing - press Play";
        else if(!processor.referencePresent.load())text="Sidechain disabled - route B to the external sidechain";
        else if(processor.inputDbA.load()<-90)text="No signal on A - play the main source";
        else if(processor.inputDbB.load()<-90)text="No signal on B - check the sidechain send";
        else if(state==1)text="Learning representative audio - keep both sources playing";
        else if(state==9)text="Section captured · Evaluate sections, or collect another passage (2–4 total)";
        else if(state==10)text="Evaluating collected audio · held correction unchanged";
        else if(state==5)text="Audio settings restored from Undo / Redo";
        else if(state==6)text="Analysis canceled - previous correction held";
        else if(state==7)text=juce::String(hearing?"HEARING PROPOSAL · not applied: ":"Proposed: ")+juce::String(processor.proposedLagMs.load(),3)+" ms · "+(processor.proposedPolarity.load()?"inverted":"normal")+" polarity · estimated +"+juce::String(processor.proposedGain.load(),3)+" interaction/correlation · Apply";
        else if(state==8){const int why=processor.recommendationReason.load();text="No useful correction: "+juce::String(why==1?"not enough kick hits":why==2?"little measurable overlap":why==3?"no meaningful improvement":why==4?"groove guard rejected timing candidates":why==5?"weak or ambiguous same-source evidence":why==6?"collect at least two sections":why==7?"one or more sections lack reliable evidence":why==8?"no common candidate safely improves the collected passages":"inconsistent or ambiguous kick relationship")+" · previous correction held";}
        else if(state==3)text="No consistent candidate across hits/windows - previous settings held";
        else if(state==4)text="Measure only - timing and polarity unchanged";
        else if(processor.settling.load())text="Applying delay / polarity correction";
        else if(processor.verified.load())text=processor.parameters.getRawParameterValue("profile")->load()<0.5f?"Output alignment verified on analysis channel":"Low-end coherence matched on analysis channel - audition A/B";
        else if(state==2)text="Learned correction held - audition A/B";
        else if(processor.parameters.getRawParameterValue("profile")->load()>0.5f)text="Ready - learn four seconds containing at least three kick hits";
        else if(reason==phasetwin::AnalysisReason::weakCorrelation)text="Sources weakly related - no safe correction found";
        else if(reason==phasetwin::AnalysisReason::ambiguous)text="Ambiguous periodic signal - play a transient passage";
        else if(reason==phasetwin::AnalysisReason::boundary)text="Delay reaches search limit - increase range (max 20 ms)";
        else if(reason==phasetwin::AnalysisReason::referenceSilent)text="Reference analysis window is silent";
        else if(reason==phasetwin::AnalysisReason::targetSilent)text="Target analysis window is silent";
        
        else text="Ready - play both sources and learn a representative section";
        if(!hearing && processor.parameters.getRawParameterValue("compare")->load()>0.5f)text+="  |  listening to NEUTRAL";
        if(processor.parameters.getRawParameterValue("freeze")->load()>0.5f)text+="  |  correction frozen";
        if(state!=7 && state!=8 && processor.parameters.getRawParameterValue("auto")->load()<0.5f)text+="  |  holding correction";
        const double rate=processor.getSampleRate();
        const double appliedMs=phasetwin::samplesToMs(processor.appliedLag.load(),rate);
        const int auditionMode=int(processor.parameters.getRawParameterValue("audition")->load());
        if(!hearing && auditionMode>0)text+=" · audition: "+juce::String(auditionMode==1?"timing only":auditionMode==2?"polarity only":"neither");
        const bool flipped=(hearing || (auditionMode!=1 && auditionMode!=3)) && ((hearing?processor.proposedPolarity.load():processor.inverted.load())!=(processor.parameters.getRawParameterValue("polarity")->load()>0.5f));
        const juce::String direction=std::abs(appliedMs)<0.0005?"no relative shift":appliedMs>0?"A advanced relative to B":"A delayed relative to B";
        auto signedMs=[](double value){return (value>0?juce::String("+"):juce::String())+juce::String(value,3)+" ms";};
        const bool neutralAudition=!hearing && processor.parameters.getRawParameterValue("compare")->load()>0.5f;
        text+=(neutralAudition?juce::String("\nStored correction: "):juce::String("\nApplied: "))+signedMs(appliedMs)+" ("+juce::String(processor.appliedLag.load(),2)+" samples)  |  "+(neutralAudition?juce::String("unaligned audition active"):direction)+(neutralAudition?"  |  Stored polarity: ":"  |  Polarity target: ")+(flipped?"INVERTED":"NORMAL");
        if(state==7)status.setTooltip("Proposed learned timing: "+juce::String(processor.proposedLagMs.load(),3)+" ms ("+juce::String(processor.proposedLagMs.load()*rate/1000,2)+" samples). Manual trim is added separately. Estimated interaction/correlation improvement: "+juce::String(processor.proposedGain.load(),3)+" (candidate measurement, not live validation). Same-source prediction uses the last captured frame and linear fractional sampling, not live verification.\nApply commits; a new Analyze or settings change invalidates this proposal.");
        text+="\nLearned: "+signedMs(phasetwin::samplesToMs(processor.detectedLag.load(),rate))+" + trim: "+signedMs(processor.parameters.getRawParameterValue("manual")->load());
        text+="  |  A "+juce::String(processor.inputDbA.load(),1)+" / B "+juce::String(processor.inputDbB.load(),1)+" dBFS  |  PDC: "+juce::String(phasetwin::samplesToMs(processor.getLatencySamples(),rate),3)+" ms";
        const juce::String residual=processor.verificationAvailable.load()?signedMs(phasetwin::samplesToMs(processor.residualLag.load(),rate))+" ("+juce::String(processor.residualLag.load(),2)+" samples)":"unavailable; kick and bass need not have matching waveforms";
        if(state!=7)status.setTooltip("Positive relative offset advances A against B; negative delays A. All output still passes through reported latency. Learned offset plus manual trim is limited to ±20 ms.\nResidual timing: "+residual+"\nOutput correlation: "+juce::String(processor.postCorrelation.load(),3)+". During application polarity is crossfaded to the target.");
        status.setText(text,juce::dontSendNotification);
        const int check=processor.verifyState.load();juce::String validation;
        if(check==0)validation="Post-Apply check: "+juce::String(processor.parameters.getRawParameterValue("verifyAfterApply")->load()>.5f?"ready after next Apply":"off");
        else if(check==1)validation="Post-Apply check: waiting for normal corrected audition and settled audio";
        else if(check==2 && idleTicks>20)validation="Post-Apply check: paused · press Play to finish measuring";
        else if(check==2)validation="Post-Apply check: measuring fresh audio · "+juce::String(processor.verifyProgress.load()*100,0)+"%";
        else{validation="Post-Apply check: "+juce::String(check==3?"IMPROVED":check==4?"WORSENED":check==5?"NO CLEAR DIFFERENCE":"NOT RELIABLY MEASURABLE");if(processor.verifyCount.load()>=3)validation+=" · unaligned "+juce::String(processor.verifyBefore.load(),3)+" → corrected "+juce::String(processor.verifyAfter.load(),3)+" · support "+juce::String(processor.verifyConfidence.load()*100,0)+"%";}
        hint.setColour(juce::Label::textColourId,check==3?PhaseTwinTheme::accent():check==4?juce::Colour(0xffffabb3):check==6?juce::Colour(0xffedbd72):juce::Colour(0xffa2b1c7));hint.setText(validation,juce::dontSendNotification);hint.setTooltip("Fresh audio comparison against unaligned A at matched latency. Kick: low-band interaction on supported kick hits; same source: signed normalized correlation. Includes phase rotation; before ducking, reference mix and output gain. No automatic changes. Support is heuristic, not a calibrated probability. A completed measurement describes that passage only; it is not continuous monitoring.");
    }
};
