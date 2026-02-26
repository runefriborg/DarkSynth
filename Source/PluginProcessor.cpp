#include "PluginProcessor.h"
#include "PluginEditor.h"

SynthPluginAudioProcessor::SynthPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    synth.addSound (new SynthSound());

    for (int i = 0; i < NUM_VOICES; ++i)
        synth.addVoice (new SynthVoice());
}

SynthPluginAudioProcessor::~SynthPluginAudioProcessor() {}

//==============================================================================
const juce::String SynthPluginAudioProcessor::getName() const      { return JucePlugin_Name; }
bool  SynthPluginAudioProcessor::acceptsMidi()  const              { return true; }
bool  SynthPluginAudioProcessor::producesMidi() const              { return false; }
bool  SynthPluginAudioProcessor::isMidiEffect() const              { return false; }
double SynthPluginAudioProcessor::getTailLengthSeconds() const     { return 2.0; }
int   SynthPluginAudioProcessor::getNumPrograms()                  { return 1; }
int   SynthPluginAudioProcessor::getCurrentProgram()               { return 0; }
void  SynthPluginAudioProcessor::setCurrentProgram (int)           {}
const juce::String SynthPluginAudioProcessor::getProgramName (int) { return {}; }
void  SynthPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void SynthPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void SynthPluginAudioProcessor::releaseResources() {}

bool SynthPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto set = layouts.getMainOutputChannelSet();
    return set == juce::AudioChannelSet::mono()
        || set == juce::AudioChannelSet::stereo();
}

//==============================================================================
void SynthPluginAudioProcessor::updateVoiceParameters()
{
    SynthParams p;
    p.waveform        = (int)  apvts.getRawParameterValue ("waveform")->load();
    p.attack          = apvts.getRawParameterValue ("attack")->load();
    p.decay           = apvts.getRawParameterValue ("decay")->load();
    p.sustain         = apvts.getRawParameterValue ("sustain")->load();
    p.release         = apvts.getRawParameterValue ("release")->load();
    p.filterCutoff    = apvts.getRawParameterValue ("filterCutoff")->load();
    p.filterResonance = apvts.getRawParameterValue ("filterResonance")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->updateParams (p);
}

void SynthPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    updateVoiceParameters();
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    // Master volume
    float vol = apvts.getRawParameterValue ("volume")->load();
    buffer.applyGain (vol);
}

//==============================================================================
bool SynthPluginAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SynthPluginAudioProcessor::createEditor()
{
    return new SynthPluginAudioProcessorEditor (*this);
}

//==============================================================================
void SynthPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SynthPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SynthPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Waveform: 0=Sine 1=Saw 2=Square 3=Triangle
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "waveform", "Waveform", 0, 3, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.05f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "decay", "Decay",
        juce::NormalisableRange<float> (0.001f, 3.0f, 0.001f, 0.5f), 0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.5f), 0.4f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 5000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterResonance", "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume", "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SynthPluginAudioProcessor();
}
