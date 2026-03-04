#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SynthPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SynthPluginAudioProcessorEditor (SynthPluginAudioProcessor&);
    ~SynthPluginAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    SynthPluginAudioProcessor& audioProcessor;

    // ---- Waveform selector ----
    juce::ComboBox waveformBox;
    juce::Label    waveformLabel;

    // ---- Sliders ----
    juce::Slider driveSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Slider focusSlider, subBlendSlider;
    juce::Slider volumeSlider;

    // ---- Labels ----
    juce::Label driveLabel;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Label focusLabel, subBlendLabel;
    juce::Label volumeLabel;

    // ---- APVTS attachments ----
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboAttach>  waveformAtt;
    std::unique_ptr<SliderAttach> driveAtt;
    std::unique_ptr<SliderAttach> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<SliderAttach> focusAtt, subBlendAtt;
    std::unique_ptr<SliderAttach> volumeAtt;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPluginAudioProcessorEditor)
};
