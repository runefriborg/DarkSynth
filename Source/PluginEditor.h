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
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Slider filterCutoffSlider, filterResSlider, volumeSlider;

    // ---- Labels ----
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Label filterCutoffLabel, filterResLabel, volumeLabel;

    // ---- APVTS attachments ----
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttach> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<SliderAttach> cutoffAtt, resonanceAtt, volumeAtt;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPluginAudioProcessorEditor)
};
