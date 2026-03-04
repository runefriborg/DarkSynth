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

    waveformBox.addItem ("Pure",  1);
    waveformBox.addItem ("Soft",  2);
    waveformBox.addItem ("Warm",  3);
    waveformBox.addItem ("Punch", 4);
    waveformBox.addItem ("Grit",  5);
    addAndMakeVisible (waveformBox);

    waveformAtt = std::make_unique<ComboAttach> (audioProcessor.apvts, "waveform", waveformBox);

    // ---- Knobs ----
    setupKnob (driveSlider,    driveLabel,    "DRIVE");
    setupKnob (attackSlider,   attackLabel,   "ATTACK");
    setupKnob (decaySlider,    decayLabel,    "DECAY");
    setupKnob (sustainSlider,  sustainLabel,  "SUSTAIN");
    setupKnob (releaseSlider,  releaseLabel,  "RELEASE");
    setupKnob (focusSlider,    focusLabel,    "FOCUS");
    setupKnob (subBlendSlider, subBlendLabel, "SUB");
    setupKnob (volumeSlider,   volumeLabel,   "VOLUME");

    // ---- Attachments ----
    driveAtt    = std::make_unique<SliderAttach> (audioProcessor.apvts, "drive",    driveSlider);
    attackAtt   = std::make_unique<SliderAttach> (audioProcessor.apvts, "attack",   attackSlider);
    decayAtt    = std::make_unique<SliderAttach> (audioProcessor.apvts, "decay",    decaySlider);
    sustainAtt  = std::make_unique<SliderAttach> (audioProcessor.apvts, "sustain",  sustainSlider);
    releaseAtt  = std::make_unique<SliderAttach> (audioProcessor.apvts, "release",  releaseSlider);
    focusAtt    = std::make_unique<SliderAttach> (audioProcessor.apvts, "focus",    focusSlider);
    subBlendAtt = std::make_unique<SliderAttach> (audioProcessor.apvts, "subBlend", subBlendSlider);
    volumeAtt   = std::make_unique<SliderAttach> (audioProcessor.apvts, "volume",   volumeSlider);
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
    g.fillRoundedRectangle (362.0f, 44.0f, 150.0f, 288.0f, 6.0f);  // Bass
    g.fillRoundedRectangle (520.0f, 44.0f, 132.0f, 288.0f, 6.0f);  // Output

    // Section header text
    g.setColour (kAccent);
    g.setFont (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold"));
    g.drawText ("OSCILLATOR", juce::Rectangle<int> (  8, 44, 140, 18), juce::Justification::centred);
    g.drawText ("ENVELOPE",   juce::Rectangle<int> (156, 44, 198, 18), juce::Justification::centred);
    g.drawText ("BASS",       juce::Rectangle<int> (362, 44, 150, 18), juce::Justification::centred);
    g.drawText ("OUTPUT",     juce::Rectangle<int> (520, 44, 132, 18), juce::Justification::centred);
}

void SynthPluginAudioProcessorEditor::resized()
{
    const int kH   = 18;  // label height
    const int kK   = 78;  // knob height
    const int kW   = 78;  // knob width
    const int kGap = 6;

    // ---- Oscillator section (x=8, w=140) ----
    int oscX = 14;
    waveformLabel.setBounds (oscX, 64, 128, kH);
    waveformBox  .setBounds (oscX, 64 + kH + 2, 128, 28);

    int driveY = 64 + kH + 2 + 28 + 16;
    driveLabel .setBounds (oscX, driveY,      kW, kH);
    driveSlider.setBounds (oscX, driveY + kH, kW, kK);

    // ---- Envelope section (x=156, w=198) — 2×2 grid ----
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

    // ---- Bass section (x=362, w=150) — stacked ----
    int bassX  = 370;
    int bassY1 = 64;
    int bassY2 = bassY1 + kH + kK + 14;

    focusLabel   .setBounds (bassX, bassY1,      kW, kH);
    focusSlider  .setBounds (bassX, bassY1 + kH, kW, kK);
    subBlendLabel.setBounds (bassX, bassY2,      kW, kH);
    subBlendSlider.setBounds (bassX, bassY2 + kH, kW, kK);

    // ---- Output section (x=520, w=132) — volume centred ----
    const int outX = 520 + (132 - kW) / 2;  // = 547
    int outY = 64;

    volumeLabel .setBounds (outX, outY,      kW, kH);
    volumeSlider.setBounds (outX, outY + kH, kW, kK);
}
