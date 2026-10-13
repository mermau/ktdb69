#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

#include "Theme.h"

//==============================================================================
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
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
    ClickToggle (juce::String on = "CLICK SELETTORE", juce::String off = "SENZA CLICK", bool alarm = false)
        : onText (on), offText (off), alarmColour (alarm) { setButtonText (on); }
    juce::String onText, offText;
    bool alarmColour;
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
    ClickToggle clickBtn, bypassBtn { "BYPASS", "BYPASS", true };
    juce::TextButton midiBtn;
    void showMidiMenu();
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAtt, outAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clickAtt, bypassAtt;
    int lastLearnShown = -1, lastCCShown = 0;
};

//==============================================================================
class DB69Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DB69Editor (DB69Processor&);
    ~DB69Editor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::theme().bar); }

private:
    void timerCallback() override;
    DB69Processor& proc;
    PanelLookAndFeel lnf;
    PanelContent content;
};
