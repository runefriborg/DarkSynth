#include "PluginEditor.h"

static const juce::Colour kBg        { 0xff1a1a2e };
static const juce::Colour kSection   { 0xff16213e };
static const juce::Colour kAccent    { 0xff4a90d9 };
static const juce::Colour kTextLight { 0xffccccdd };

SynthPluginAudioProcessorEditor::SynthPluginAudioProcessorEditor (SynthPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (660, 340);

    // ---- Waveform combo ----
    waveformLabel.setText ("WAVEFORM", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centredLeft);
    waveformLabel.setColour (juce::Label::textColourId, kAccent);
    addAndMakeVisible (waveformLabel);

    waveformBox.addItem ("Sine",     1);
    waveformBox.addItem ("Sawtooth", 2);
    waveformBox.addItem ("Square",   3);
    waveformBox.addItem ("Triangle", 4);
    waveformBox.addItem ("SuperSaw", 5);
    addAndMakeVisible (waveformBox);

    // ComboBoxAttachment maps item index (0-based) → parameter value via convertTo0to1
    waveformAtt = std::make_unique<ComboAttach> (audioProcessor.apvts, "waveform", waveformBox);

    // ---- Knobs ----
    setupKnob (superSawDetuneSlider, superSawDetuneLabel, "DETUNE");
    setupKnob (attackSlider,         attackLabel,         "ATTACK");
    setupKnob (decaySlider,          decayLabel,          "DECAY");
    setupKnob (sustainSlider,        sustainLabel,        "SUSTAIN");
    setupKnob (releaseSlider,        releaseLabel,        "RELEASE");
    setupKnob (filterCutoffSlider,   filterCutoffLabel,   "CUTOFF");
    setupKnob (filterResSlider,      filterResLabel,      "RESONANCE");
    setupKnob (volumeSlider,         volumeLabel,         "VOLUME");
    setupKnob (unisonVoicesSlider,   unisonVoicesLabel,   "VOICES");
    setupKnob (unisonDetuneSlider,   unisonDetuneLabel,   "SPREAD");

    // Voices snaps to integers — hide decimals
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);

    // ---- Attachments ----
    attackAtt        = std::make_unique<SliderAttach> (audioProcessor.apvts, "attack",          attackSlider);
    decayAtt         = std::make_unique<SliderAttach> (audioProcessor.apvts, "decay",           decaySlider);
    sustainAtt       = std::make_unique<SliderAttach> (audioProcessor.apvts, "sustain",         sustainSlider);
    releaseAtt       = std::make_unique<SliderAttach> (audioProcessor.apvts, "release",         releaseSlider);
    cutoffAtt        = std::make_unique<SliderAttach> (audioProcessor.apvts, "filterCutoff",    filterCutoffSlider);
    resonanceAtt     = std::make_unique<SliderAttach> (audioProcessor.apvts, "filterResonance", filterResSlider);
    volumeAtt        = std::make_unique<SliderAttach> (audioProcessor.apvts, "volume",          volumeSlider);
    superSawDetuneAtt = std::make_unique<SliderAttach> (audioProcessor.apvts, "superSawDetune", superSawDetuneSlider);
    unisonVoicesAtt  = std::make_unique<SliderAttach> (audioProcessor.apvts, "unisonVoices",   unisonVoicesSlider);
    unisonDetuneAtt  = std::make_unique<SliderAttach> (audioProcessor.apvts, "unisonDetune",   unisonDetuneSlider);
}

SynthPluginAudioProcessorEditor::~SynthPluginAudioProcessorEditor() {}

void SynthPluginAudioProcessorEditor::setupKnob (juce::Slider& slider,
                                                   juce::Label& label,
                                                   const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId,    kAccent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333355));
    slider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    slider.setColour (juce::Slider::textBoxTextColourId,         kTextLight);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions{}.withHeight (10.5f).withStyle ("Bold"));
    label.setColour (juce::Label::textColourId, kTextLight);
    addAndMakeVisible (label);
}

void SynthPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (kBg);

    // Header
    g.setColour (kAccent);
    g.setFont (juce::FontOptions{}.withHeight (20.0f).withStyle ("Bold"));
    g.drawText ("DARKSYNTH", juce::Rectangle<int> (0, 0, getWidth(), 38),
                juce::Justification::centred);

    // Thin accent line under header
    g.setColour (kAccent.withAlpha (0.5f));
    g.fillRect (0, 36, getWidth(), 1);

    // Section backgrounds
    g.setColour (kSection);
    g.fillRoundedRectangle (  8.0f, 44.0f, 140.0f, 288.0f, 6.0f);  // Oscillator
    g.fillRoundedRectangle (156.0f, 44.0f, 198.0f, 288.0f, 6.0f);  // Envelope
    g.fillRoundedRectangle (362.0f, 44.0f, 150.0f, 288.0f, 6.0f);  // Filter + Vol
    g.fillRoundedRectangle (520.0f, 44.0f, 132.0f, 288.0f, 6.0f);  // Unison

    // Section header text
    g.setColour (kAccent);
    g.setFont (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold"));
    g.drawText ("OSCILLATOR",   juce::Rectangle<int> (  8, 44, 140, 18), juce::Justification::centred);
    g.drawText ("ENVELOPE",     juce::Rectangle<int> (156, 44, 198, 18), juce::Justification::centred);
    g.drawText ("FILTER / VOL", juce::Rectangle<int> (362, 44, 150, 18), juce::Justification::centred);
    g.drawText ("UNISON",       juce::Rectangle<int> (520, 44, 132, 18), juce::Justification::centred);
}

void SynthPluginAudioProcessorEditor::resized()
{
    const int kH   = 18;  // label height
    const int kK   = 78;  // knob height
    const int kW   = 78;  // knob width
    const int kGap = 6;

    // ---- Oscillator section ----
    int oscX = 14;
    waveformLabel.setBounds (oscX, 64, 128, kH);
    waveformBox  .setBounds (oscX, 64 + kH + 2, 128, 28);

    int volY = 64 + kH + 2 + 28 + 16;
    volumeLabel .setBounds (oscX, volY,      kW, kH);
    volumeSlider.setBounds (oscX, volY + kH, kW, kK);

    int detuneY = volY + kH + kK + 6;
    superSawDetuneLabel .setBounds (oscX, detuneY,      kW, kH);
    superSawDetuneSlider.setBounds (oscX, detuneY + kH, kW, kK);

    // ---- Envelope section (2×2 grid) ----
    int envX1 = 162;
    int envX2 = envX1 + kW + kGap + 4;
    int envY1 = 64;
    int envY2 = envY1 + kH + kK + 14;

    attackLabel  .setBounds (envX1, envY1,      kW, kH);
    attackSlider .setBounds (envX1, envY1 + kH, kW, kK);
    decayLabel   .setBounds (envX2, envY1,      kW, kH);
    decaySlider  .setBounds (envX2, envY1 + kH, kW, kK);
    sustainLabel .setBounds (envX1, envY2,      kW, kH);
    sustainSlider.setBounds (envX1, envY2 + kH, kW, kK);
    releaseLabel .setBounds (envX2, envY2,      kW, kH);
    releaseSlider.setBounds (envX2, envY2 + kH, kW, kK);

    // ---- Filter section (stacked) ----
    int filtX  = 370;
    int filtY1 = 64;
    int filtY2 = filtY1 + kH + kK + 14;

    filterCutoffLabel .setBounds (filtX, filtY1,      kW, kH);
    filterCutoffSlider.setBounds (filtX, filtY1 + kH, kW, kK);
    filterResLabel    .setBounds (filtX, filtY2,      kW, kH);
    filterResSlider   .setBounds (filtX, filtY2 + kH, kW, kK);

    // ---- Unison section (stacked) ----
    // Centre knobs inside the 132px-wide section (x=520)
    const int uniX  = 520 + (132 - kW) / 2;  // = 547
    int uniY1 = 64;
    int uniY2 = uniY1 + kH + kK + 14;

    unisonVoicesLabel .setBounds (uniX, uniY1,      kW, kH);
    unisonVoicesSlider.setBounds (uniX, uniY1 + kH, kW, kK);
    unisonDetuneLabel .setBounds (uniX, uniY2,      kW, kH);
    unisonDetuneSlider.setBounds (uniX, uniY2 + kH, kW, kK);
}
