#pragma once
#include "JuceLibraryCode/JuceHeader.h"

namespace onebite {

// Color palette
namespace Colors {
    const juce::Colour background   { 0xff1a1a2e };
    const juce::Colour panelBg      { 0xff16213e };
    const juce::Colour headerBg     { 0xff0f3460 };
    const juce::Colour textPrimary  { 0xffe0e0e0 };
    const juce::Colour textDim      { 0xff808090 };
    const juce::Colour knobArc      { 0xff4a90d9 };
    const juce::Colour stepOff      { 0xff2a2a3e };
    const juce::Colour trackA       { 0xff00d4ff }; // cyan
    const juce::Colour trackB       { 0xffff00ff }; // magenta
    const juce::Colour trackC       { 0xffffd700 }; // yellow
    const juce::Colour trackD       { 0xff00ff88 }; // green
    const juce::Colour gateRed      { 0xffff3333 };
    const juce::Colour modeActive   { 0xff4a90d9 };
    const juce::Colour modeInactive { 0xff2a2a3e };
}

//==============================================================================
class OneBiteLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OneBiteLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
};

//==============================================================================
class StepButton : public juce::Component
{
public:
    StepButton(juce::RangedAudioParameter* param, juce::Colour colour);
    ~StepButton() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter* parameter;
    juce::Colour activeColour;
    bool isOn = false;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
class ModeSelector : public juce::Component
{
public:
    ModeSelector(juce::RangedAudioParameter* param);
    ~ModeSelector() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter* parameter;
    int currentMode = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    static constexpr const char* labels[] = { "BPM", "HZ", "SMP" };
};

//==============================================================================
class GateButton : public juce::Component
{
public:
    GateButton(juce::RangedAudioParameter* param);
    ~GateButton() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter* parameter;
    bool pressed = false;
};

//==============================================================================
class TrackRow : public juce::Component
{
public:
    TrackRow(juce::AudioProcessor& proc, const juce::String& trackName,
             int numSteps, juce::Colour colour);
    void resized() override;

private:
    juce::RangedAudioParameter* findParam(juce::AudioProcessor& proc,
                                          const juce::String& paramId);

    juce::Label nameLabel;
    juce::Slider volKnob, panKnob;
    juce::ToggleButton rndButton { "rnd" }, clrButton { "clr" };
    juce::OwnedArray<StepButton> stepButtons;

    std::unique_ptr<juce::SliderParameterAttachment> volAttach, panAttach;
    std::unique_ptr<juce::ButtonParameterAttachment> rndAttach, clrAttach;

    juce::Colour trackColour;
};

//==============================================================================
class OneBiteEditor : public juce::AudioProcessorEditor
{
public:
    explicit OneBiteEditor(juce::AudioProcessor&);
    ~OneBiteEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::RangedAudioParameter* findParam(const juce::String& paramId);

    OneBiteLookAndFeel lnf;

    // Header
    ModeSelector modeSelector;
    juce::Slider rateKnob;
    std::unique_ptr<juce::SliderParameterAttachment> rateAttach;

    // Tracks
    TrackRow trackA, trackB, trackC, trackD;

    // Footer
    juce::Slider driveKnob, dryWetKnob;
    juce::ToggleButton bypassButton { "bypass" };
    GateButton gateButton;

    std::unique_ptr<juce::SliderParameterAttachment> driveAttach, dryWetAttach;
    std::unique_ptr<juce::ButtonParameterAttachment> bypassAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneBiteEditor)
};

} // namespace onebite
