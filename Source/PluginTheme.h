#pragma once
// Editor-only styling. No global LookAndFeel and no changes to audio parameters.
class PhaseTwinTheme final : public juce::LookAndFeel_V4 {
public:
    static juce::Colour accent(){return juce::Colour(0xff67e8c1);}
    PhaseTwinTheme(){
        setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xff405b70));
        setColour(juce::Label::textColourId,juce::Colour(0xffc5cfdf));
        setColour(juce::TextButton::buttonColourId,juce::Colour(0xff202c3c));
        setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff254c48));
        setColour(juce::TextButton::textColourOffId,juce::Colour(0xffdce6f3));
        setColour(juce::TextButton::textColourOnId,accent());
        setColour(juce::ToggleButton::textColourId,juce::Colour(0xffc5cfdf));
        setColour(juce::ToggleButton::tickColourId,accent());
        setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(0xff5b6b80));
        setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff121c29));
        setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff33445b));
        setColour(juce::ComboBox::textColourId,juce::Colour(0xffe0e8f3));
        setColour(juce::ComboBox::arrowColourId,accent());
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff182334));
        setColour(juce::PopupMenu::textColourId,juce::Colour(0xffe0e8f3));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff254c48));
        setColour(juce::PopupMenu::highlightedTextColourId,accent());
        setColour(juce::Slider::trackColourId,accent());
        setColour(juce::Slider::backgroundColourId,juce::Colour(0xff0c1420));
        setColour(juce::Slider::thumbColourId,juce::Colour(0xffe1fff6));
        setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101a27));
        setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe0e8f3));
        setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff33445b));
        setColour(juce::TextEditor::highlightColourId,juce::Colour(0xff254c48));
        setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff202c3c));
        setColour(juce::TooltipWindow::textColourId,juce::Colour(0xffe0e8f3));
        setColour(juce::TooltipWindow::outlineColourId,juce::Colour(0xff405570));
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour& colour,bool hover,bool down)override{
        auto bounds=button.getLocalBounds().toFloat().reduced(.5f);
        auto base=button.getToggleState()?button.findColour(juce::TextButton::buttonOnColourId):colour;
        if(hover)base=base.brighter(.12f);
        if(down)base=base.darker(.14f);
        if(!button.isEnabled())base=base.withAlpha(.4f);
        g.setGradientFill(juce::ColourGradient(base.brighter(.07f),0,bounds.getY(),base,0,bounds.getBottom(),false));
        g.fillRoundedRectangle(bounds,6);
        g.setColour(button.hasKeyboardFocus(true)?accent():juce::Colour(0xff405570).withAlpha(button.isEnabled()?.65f:.25f));
        g.drawRoundedRectangle(bounds,6,button.hasKeyboardFocus(true)?1.5f:1.0f);
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float sliderPos,float minPos,float maxPos,const juce::Slider::SliderStyle style,juce::Slider& slider)override{
        if(style!=juce::Slider::LinearHorizontal){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,sliderPos,minPos,maxPos,style,slider);return;}
        const float cy=float(y)+float(height)*.5f,left=float(x),right=float(x+width);
        const float pos=juce::jlimit(left,right,sliderPos),opacity=slider.isEnabled()?1.0f:.35f;
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));g.fillRoundedRectangle(left,cy-3,float(width),6,3);
        g.setColour(slider.findColour(juce::Slider::trackColourId).withAlpha(opacity));g.fillRoundedRectangle(left,cy-3,std::max(0.0f,pos-left),6,3);
        g.setColour(juce::Colour(0xff09121e));g.fillEllipse(pos-7,cy-7,14,14);
        g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(opacity));g.fillEllipse(pos-5,cy-5,10,10);
        if(slider.hasKeyboardFocus(true)){g.setColour(accent());g.drawEllipse(pos-8,cy-8,16,16,1);}
    }
};
