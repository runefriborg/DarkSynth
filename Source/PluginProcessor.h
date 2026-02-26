#pragma once
#include <JuceHeader.h>
#include "UnisonSynthesiser.h"

class SynthPluginAudioProcessor : public juce::AudioProcessor
{
public:
    SynthPluginAudioProcessor();
    ~SynthPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool  acceptsMidi()  const override;
    bool  producesMidi() const override;
    bool  isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms()  override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int NUM_VOICES = 16;

    UnisonSynthesiser synth;
    int currentProgram = 0;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateVoiceParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPluginAudioProcessor)
};
