#include "OneBiteEditor.h"
#include <fstream>

namespace onebite {

// Debug log file
static std::ofstream& debugLog()
{
    static std::ofstream log("C:/Users/lpfre/OneDrive/Desktop/onebite_debug.txt", std::ios::app);
    return log;
}

//==============================================================================
// Helper: find a parameter by its string ID
//==============================================================================
static juce::RangedAudioParameter* findParameter(juce::AudioProcessor& proc,
                                                   const juce::String& paramId)
{
    for (auto* p : proc.getParameters())
    {
        if (auto* rap = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            if (rap->getParameterID() == paramId)
                return rap;
        }
    }
    debugLog() << "PARAM NOT FOUND: " << paramId.toStdString() << std::endl;
    return nullptr;
}

//==============================================================================
// OneBiteLookAndFeel
//==============================================================================
OneBiteLookAndFeel::OneBiteLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, Colors::knobArc);
    setColour(juce::Slider::rotarySliderOutlineColourId, Colors::stepOff);
    setColour(juce::Slider::thumbColourId, Colors::textPrimary);
    setColour(juce::Label::textColourId, Colors::textPrimary);
    setColour(juce::ToggleButton::textColourId, Colors::textPrimary);
    setColour(juce::ToggleButton::tickColourId, Colors::knobArc);
}

void OneBiteLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                           int w, int h, float sliderPos,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, w, h).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto lineW = 2.5f;
    auto arcRadius = radius - lineW * 0.5f;

    // Background arc
    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(Colors::stepOff);
    g.strokePath(bgArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Value arc
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, toAngle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Thumb dot
    juce::Point<float> thumbPoint(
        centre.x + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
        centre.y + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));
    g.setColour(Colors::textPrimary);
    g.fillEllipse(juce::Rectangle<float>(6.0f, 6.0f).withCentre(thumbPoint));
}

void OneBiteLookAndFeel::drawToggleButton(juce::Graphics& g,
                                           juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    auto isOn = button.getToggleState();

    g.setColour(isOn ? Colors::knobArc.withAlpha(0.3f) : Colors::stepOff);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(isOn ? Colors::knobArc : Colors::textDim);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    g.setColour(isOn ? Colors::textPrimary : Colors::textDim);
    g.setFont(juce::Font(11.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(bounds, 4.0f);
    }
}

//==============================================================================
// StepButton
//==============================================================================
StepButton::StepButton(juce::RangedAudioParameter* param, juce::Colour colour)
    : parameter(param), activeColour(colour)
{
    if (parameter != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment>(
            *parameter,
            [this](float newValue)
            {
                isOn = newValue >= 0.5f;
                repaint();
            },
            nullptr);
        attachment->sendInitialUpdate();
    }
}

StepButton::~StepButton() = default;

void StepButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.5f);

    if (isOn)
    {
        g.setColour(activeColour.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(activeColour);
        g.drawRoundedRectangle(bounds, 3.0f, 1.5f);

        // Inner glow
        auto inner = bounds.reduced(4.0f);
        g.setColour(activeColour.withAlpha(0.6f));
        g.fillRoundedRectangle(inner, 2.0f);
    }
    else
    {
        g.setColour(Colors::stepOff);
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(Colors::textDim.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }
}

void StepButton::mouseDown(const juce::MouseEvent&)
{
    debugLog() << "StepButton::mouseDown param=" << (parameter ? parameter->getParameterID().toStdString() : "NULL")
               << " isOn=" << isOn << std::endl;
    if (parameter != nullptr)
    {
        isOn = !isOn;
        attachment->setValueAsCompleteGesture(isOn ? 1.0f : 0.0f);
        debugLog() << "  -> set to " << (isOn ? 1.0f : 0.0f)
                   << " paramValue now=" << parameter->getValue() << std::endl;
        repaint();
    }
}

//==============================================================================
// ModeSelector
//==============================================================================
ModeSelector::ModeSelector(juce::RangedAudioParameter* param)
    : parameter(param)
{
    if (parameter != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment>(
            *parameter,
            [this](float newValue)
            {
                currentMode = juce::roundToInt(newValue);
                repaint();
            },
            nullptr);
        attachment->sendInitialUpdate();
    }
}

ModeSelector::~ModeSelector() = default;

void ModeSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto segW = bounds.getWidth() / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        auto seg = bounds.withX(bounds.getX() + i * segW).withWidth(segW);
        bool active = (i == currentMode);

        g.setColour(active ? Colors::modeActive : Colors::modeInactive);
        g.fillRoundedRectangle(seg.reduced(1.0f), 3.0f);

        g.setColour(active ? Colors::textPrimary : Colors::textDim);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(labels[i], seg, juce::Justification::centred);
    }

    g.setColour(Colors::textDim.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void ModeSelector::mouseDown(const juce::MouseEvent& e)
{
    if (parameter == nullptr) return;

    auto segW = getWidth() / 3.0f;
    int clicked = juce::jlimit(0, 2, (int)(e.position.x / segW));
    if (clicked != currentMode)
    {
        currentMode = clicked;
        attachment->setValueAsCompleteGesture((float)currentMode);
        repaint();
    }
}

//==============================================================================
// GateButton
//==============================================================================
GateButton::GateButton(juce::RangedAudioParameter* param)
    : parameter(param)
{
}

GateButton::~GateButton() = default;

void GateButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    g.setColour(pressed ? Colors::gateRed : Colors::stepOff);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(pressed ? Colors::gateRed.brighter(0.3f) : Colors::textDim);
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    g.setColour(pressed ? Colors::textPrimary : Colors::textDim);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("GATE", bounds, juce::Justification::centred);
}

void GateButton::mouseDown(const juce::MouseEvent&)
{
    if (parameter == nullptr) return;
    pressed = true;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(1.0f);
    repaint();
}

void GateButton::mouseUp(const juce::MouseEvent&)
{
    if (parameter == nullptr) return;
    pressed = false;
    parameter->setValueNotifyingHost(0.0f);
    parameter->endChangeGesture();
    repaint();
}

//==============================================================================
// TrackRow
//==============================================================================
TrackRow::TrackRow(juce::AudioProcessor& proc, const juce::String& trackName,
                   int numSteps, juce::Colour colour)
    : trackColour(colour)
{
    // Track label
    nameLabel.setText(trackName, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, colour);
    nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel);

    // Vol knob
    volKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volKnob.setColour(juce::Slider::rotarySliderFillColourId, colour);
    addAndMakeVisible(volKnob);
    if (auto* p = findParam(proc, "/Polyphonic/Voices/one_bite/" + trackName + "/ctrl/vol"))
        volAttach = std::make_unique<juce::SliderParameterAttachment>(*p, volKnob);

    // Pan knob
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    panKnob.setColour(juce::Slider::rotarySliderFillColourId, colour);
    addAndMakeVisible(panKnob);
    if (auto* p = findParam(proc, "/Polyphonic/Voices/one_bite/" + trackName + "/ctrl/pan"))
        panAttach = std::make_unique<juce::SliderParameterAttachment>(*p, panKnob);

    // Rnd toggle
    addAndMakeVisible(rndButton);
    if (auto* p = findParam(proc, "/Polyphonic/Voices/one_bite/" + trackName + "/ctrl/rnd"))
        rndAttach = std::make_unique<juce::ButtonParameterAttachment>(*p, rndButton);

    // Clr toggle
    addAndMakeVisible(clrButton);
    if (auto* p = findParam(proc, "/Polyphonic/Voices/one_bite/" + trackName + "/ctrl/clr"))
        clrAttach = std::make_unique<juce::ButtonParameterAttachment>(*p, clrButton);

    // Step buttons
    debugLog() << "TrackRow " << trackName.toStdString() << " creating " << numSteps << " steps:" << std::endl;
    for (int i = 0; i < numSteps; ++i)
    {
        auto stepId = juce::String("/Polyphonic/Voices/one_bite/") + trackName + "/pat/"
                      + juce::String(i + 1).paddedLeft('0', 2);
        auto* param = findParam(proc, stepId);
        debugLog() << "  step " << stepId.toStdString() << " -> " << (param ? "FOUND" : "NULL") << std::endl;
        auto* sb = stepButtons.add(new StepButton(param, colour));
        addAndMakeVisible(sb);
    }
}

void TrackRow::resized()
{
    auto area = getLocalBounds();
    auto controlH = area.getHeight() / 2;  // top half: controls, bottom half: steps

    auto top = area.removeFromTop(controlH);
    nameLabel.setBounds(top.removeFromLeft(30));
    volKnob.setBounds(top.removeFromLeft(40));
    panKnob.setBounds(top.removeFromLeft(40));
    rndButton.setBounds(top.removeFromLeft(36).reduced(2));
    clrButton.setBounds(top.removeFromLeft(36).reduced(2));

    // Steps fill the bottom row
    auto bottom = area;
    int numSteps = stepButtons.size();
    if (numSteps == 0) return;

    int stepW = juce::jmin(30, bottom.getWidth() / numSteps);
    int totalW = stepW * numSteps;
    int startX = bottom.getX() + 30; // align with after the name label

    for (int i = 0; i < numSteps; ++i)
    {
        stepButtons[i]->setBounds(startX + i * stepW, bottom.getY(), stepW,
                                   bottom.getHeight());
    }
}

juce::RangedAudioParameter* TrackRow::findParam(juce::AudioProcessor& proc,
                                                  const juce::String& paramId)
{
    return findParameter(proc, paramId);
}

//==============================================================================
// OneBiteEditor
//==============================================================================
OneBiteEditor::OneBiteEditor(juce::AudioProcessor& p)
    : juce::AudioProcessorEditor(p),
      modeSelector(findParam("/Polyphonic/Voices/one_bite/clock/mode/mode")),
      gateButton(findParam("/Polyphonic/Voices/one_bite/gate")),
      trackA(p, "A", 16, Colors::trackA),
      trackB(p, "B", 8,  Colors::trackB),
      trackC(p, "C", 4,  Colors::trackC),
      trackD(p, "D", 2,  Colors::trackD)
{
    setLookAndFeel(&lnf);
    setSize(780, 520);
    setResizable(true, true);
    setResizeLimits(600, 400, 1200, 800);

    // DEBUG: dump all parameter IDs to file
    {
        juce::File dbgFile(juce::File::getSpecialLocation(
            juce::File::userDesktopDirectory).getChildFile("onebite_params.txt"));
        juce::String txt;
        for (auto* param : p.getParameters())
        {
            if (auto* rap = dynamic_cast<juce::RangedAudioParameter*>(param))
                txt += rap->getParameterID() + "  val=" + juce::String(rap->getValue()) + "\n";
            else
                txt += "(non-ranged param)\n";
        }
        dbgFile.replaceWithText(txt);
    }

    // Header - mode selector
    addAndMakeVisible(modeSelector);

    // Header - rate knob
    rateKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    rateKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(rateKnob);
    if (auto* p2 = findParam("/Polyphonic/Voices/one_bite/clock/rate/rate"))
        rateAttach = std::make_unique<juce::SliderParameterAttachment>(*p2, rateKnob);

    // Tracks
    addAndMakeVisible(trackA);
    addAndMakeVisible(trackB);
    addAndMakeVisible(trackC);
    addAndMakeVisible(trackD);

    // Footer - distortion
    driveKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(driveKnob);
    if (auto* p2 = findParam("/Polyphonic/Voices/one_bite/global/dist/drive"))
        driveAttach = std::make_unique<juce::SliderParameterAttachment>(*p2, driveKnob);

    dryWetKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(dryWetKnob);
    if (auto* p2 = findParam("/Polyphonic/Voices/one_bite/global/dist/drywet"))
        dryWetAttach = std::make_unique<juce::SliderParameterAttachment>(*p2, dryWetKnob);

    addAndMakeVisible(bypassButton);
    if (auto* p2 = findParam("/Polyphonic/Voices/one_bite/global/dist/bypass"))
        bypassAttach = std::make_unique<juce::ButtonParameterAttachment>(*p2, bypassButton);

    addAndMakeVisible(gateButton);
}

OneBiteEditor::~OneBiteEditor()
{
    setLookAndFeel(nullptr);
}

void OneBiteEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colors::background);

    // Header bar
    auto headerArea = getLocalBounds().removeFromTop(50);
    g.setColour(Colors::headerBg);
    g.fillRect(headerArea);

    // Title
    g.setColour(Colors::textPrimary);
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("1-BITE", headerArea.removeFromLeft(120), juce::Justification::centred);

    // Track separators
    auto bodyArea = getLocalBounds().withTrimmedTop(50).withTrimmedBottom(55);
    int trackH = bodyArea.getHeight() / 4;

    g.setColour(Colors::panelBg.brighter(0.05f));
    for (int i = 1; i < 4; ++i)
    {
        int yy = bodyArea.getY() + i * trackH;
        g.drawHorizontalLine(yy, (float)bodyArea.getX(), (float)bodyArea.getRight());
    }

    // Footer bar
    auto footerArea = getLocalBounds().removeFromBottom(55);
    g.setColour(Colors::panelBg);
    g.fillRect(footerArea);

    // Footer labels
    g.setColour(Colors::textDim);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("DIST", footerArea.removeFromLeft(45), juce::Justification::centred);
}

void OneBiteEditor::resized()
{
    auto area = getLocalBounds();

    // Header
    auto header = area.removeFromTop(50);
    header.removeFromLeft(120); // skip title
    modeSelector.setBounds(header.removeFromLeft(150).reduced(8));
    rateKnob.setBounds(header.removeFromLeft(50).reduced(5));

    // Footer
    auto footer = area.removeFromBottom(55);
    footer.removeFromLeft(45); // skip DIST label
    driveKnob.setBounds(footer.removeFromLeft(50).reduced(5));
    dryWetKnob.setBounds(footer.removeFromLeft(50).reduced(5));
    bypassButton.setBounds(footer.removeFromLeft(55).reduced(5));
    gateButton.setBounds(footer.removeFromRight(80).reduced(5));

    // Tracks
    int trackH = area.getHeight() / 4;
    trackA.setBounds(area.removeFromTop(trackH));
    trackB.setBounds(area.removeFromTop(trackH));
    trackC.setBounds(area.removeFromTop(trackH));
    trackD.setBounds(area);
}

juce::RangedAudioParameter* OneBiteEditor::findParam(const juce::String& paramId)
{
    return findParameter(processor, paramId);
}

} // namespace onebite
