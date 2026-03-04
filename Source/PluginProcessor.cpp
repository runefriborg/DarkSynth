#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Factory presets
//==============================================================================
namespace {
    struct Preset {
        const char* name;
        float waveform, attack, decay, sustain, release;
        float focus, drive, subBlend, volume;
    };

    constexpr Preset kPresets[] = {
        //  name           wave  att    dec    sus    rel    focus  drv   sub   vol
        { "Init",          0,  0.01f, 0.30f, 0.70f, 0.20f,  3.0f, 0.0f, 0.00f, 0.70f },
        { "Deep Thump",    1,  0.01f, 0.15f, 0.00f, 0.30f,  5.0f, 0.0f, 0.35f, 0.70f },
        { "Sub Bass",      0,  0.20f, 0.40f, 0.60f, 0.80f,  3.0f, 0.0f, 0.80f, 0.70f },
        { "Warm Tone",     2,  0.02f, 0.30f, 0.65f, 0.35f,  3.5f, 0.2f, 0.15f, 0.70f },
        { "Grit Bass",     4,  0.01f, 0.12f, 0.00f, 0.20f,  3.0f, 0.8f, 0.10f, 0.70f },
    };

    constexpr int kNumPresets = (int) std::size (kPresets);
}

//==============================================================================
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

int  SynthPluginAudioProcessor::getNumPrograms()    { return kNumPresets; }
int  SynthPluginAudioProcessor::getCurrentProgram() { return currentProgram; }
void SynthPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

const juce::String SynthPluginAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumPresets) return {};
    return kPresets[index].name;
}

void SynthPluginAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= kNumPresets) return;
    currentProgram = index;

    const auto& p = kPresets[index];

    auto setP = [this] (const char* id, float v) {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (v));
    };

    setP ("waveform",  p.waveform);
    setP ("attack",    p.attack);
    setP ("decay",     p.decay);
    setP ("sustain",   p.sustain);
    setP ("release",   p.release);
    setP ("focus",     p.focus);
    setP ("drive",     p.drive);
    setP ("subBlend",  p.subBlend);
    setP ("volume",    p.volume);
}

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
    p.waveform = (int) apvts.getRawParameterValue ("waveform")->load();
    p.attack   = apvts.getRawParameterValue ("attack")->load();
    p.decay    = apvts.getRawParameterValue ("decay")->load();
    p.sustain  = apvts.getRawParameterValue ("sustain")->load();
    p.release  = apvts.getRawParameterValue ("release")->load();
    p.focus    = apvts.getRawParameterValue ("focus")->load();
    p.drive    = apvts.getRawParameterValue ("drive")->load();
    p.subBlend = apvts.getRawParameterValue ("subBlend")->load();

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

    // Waveform: 0=Pure 1=Soft 2=Warm 3=Punch 4=Grit
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "waveform", "Waveform", 0, 4, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "decay", "Decay",
        juce::NormalisableRange<float> (0.001f, 3.0f, 0.001f, 0.5f), 0.30f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.70f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.5f), 0.20f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "focus", "Focus",
        juce::NormalisableRange<float> (1.0f, 8.0f, 0.01f), 3.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "drive", "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "subBlend", "Sub Blend",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume", "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.70f));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SynthPluginAudioProcessor();
}
