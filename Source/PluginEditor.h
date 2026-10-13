#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace ui
{
    const juce::Colour cream  { 0xffe9e2cc };
    const juce::Colour panel  { 0xff3b4832 };
    const juce::Colour panelD { 0xff2c3626 };
    const juce::Colour bar    { 0xff1a1b19 };
    const juce::Colour amber  { 0xffe39a3b };
}

//==============================================================================
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
// Selettore rotativo a scatti (manopola grande, MODE, BELL) disegnato come sul pannello
class SteppedKnob : public juce::Component
{
public:
    enum class Style { Big, Mode, Bell };

    SteppedKnob (juce::RangedAudioParameter& p, Style s, std::vector<float> anglesDeg,
                 std::vector<juce::String> labels, juce::Point<float> centre, float radius);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool hitTest (int x, int y) override;

    std::function<void()> onLearnMenu;
    bool learning = false, wasLearning = false;
    int getIndex() const { return index; }

private:
    void setIndex (int i, bool notify);
    int labelAt (juce::Point<float> p) const;
    juce::Point<float> polar (float r, float deg) const;

    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    Style style;
    std::vector<float> angles;
    std::vector<juce::String> labels;
    juce::Point<float> c;
    float radius;
    int index = 0, dragStartIndex = 0;
    float dragAccum = 0.0f;
    bool dragging = false, popupClick = false;
    float wheelAccum = 0.0f;
};

// Slider di servizio (INPUT/OUTPUT) con MIDI Learn sul clic destro
class LearnableSlider : public juce::Slider
{
public:
    std::function<void()> onLearnMenu;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) Slider::mouseDrag (e); }
    void mouseUp   (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) Slider::mouseUp (e); }
};

class ClickToggle : public juce::ToggleButton
{
public:
    ClickToggle() { setButtonText ("CLICK"); }
    std::function<void()> onLearnMenu;
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) ToggleButton::mouseUp (e); }
};

//==============================================================================
// Tutto il contenuto alle dimensioni di progetto; l'editor lo scala
class PanelContent : public juce::Component
{
public:
    explicit PanelContent (DB69Processor&);
    ~PanelContent() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;

    static constexpr int baseW = 780, baseH = 500, barH = 44;
    void refresh();

private:
    void showLearnMenu (int idx, juce::Component* target);
    void drawPanel (juce::Graphics&);
    juce::String ccText (int idx) const;

    DB69Processor& proc;
    std::unique_ptr<SteppedKnob> bigKnob, modeKnob, bellKnob;
    LearnableSlider inSlider, outSlider;
    ClickToggle clickBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAtt, outAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clickAtt;
    int lastLearnShown = -1;
};

//==============================================================================
class DB69Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DB69Editor (DB69Processor&);
    ~DB69Editor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::bar); }

private:
    void timerCallback() override;
    DB69Processor& proc;
    PanelLookAndFeel lnf;
    PanelContent content;
    int lastLearn = -2, lastMapVersion = -1, lastCCCount = -1;
};
