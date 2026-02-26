#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Factory presets
//==============================================================================
namespace {
    struct Preset {
        const char* name;
        float waveform, attack, decay, sustain, release;
        float filterCutoff, filterResonance, volume;
        float superSawDetune, unisonVoices, unisonDetune;
    };

    constexpr Preset kPresets[] = {
        //  name             wave  att    dec    sus    rel    cutoff   res   vol   ssDet  uniV  uniDet
        { "Init",            0,  0.050f, 0.100f, 0.800f, 0.400f,  5000.f, 0.70f, 0.70f, 0.30f, 1.f, 0.10f },
        { "SuperSaw Pad",    4,  0.300f, 0.200f, 0.850f, 1.500f,  7000.f, 0.40f, 0.65f, 0.60f, 4.f, 0.12f },
        { "Saw Lead",        1,  0.005f, 0.100f, 0.700f, 0.150f,  6000.f, 1.20f, 0.70f, 0.30f, 1.f, 0.00f },
        { "Bass Pluck",      2,  0.001f, 0.400f, 0.000f, 0.200f,   800.f, 2.00f, 0.75f, 0.30f, 1.f, 0.00f },
        { "Ambient Drift",   0,  2.000f, 0.300f, 0.700f, 3.000f,  2500.f, 0.50f, 0.60f, 0.30f, 2.f, 0.25f },
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

    setP ("waveform",        p.waveform);
    setP ("attack",          p.attack);
    setP ("decay",           p.decay);
    setP ("sustain",         p.sustain);
    setP ("release",         p.release);
    setP ("filterCutoff",    p.filterCutoff);
    setP ("filterResonance", p.filterResonance);
    setP ("volume",          p.volume);
    setP ("superSawDetune",  p.superSawDetune);
    setP ("unisonVoices",    p.unisonVoices);
    setP ("unisonDetune",    p.unisonDetune);
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
    p.waveform        = (int)  apvts.getRawParameterValue ("waveform")->load();
    p.attack          = apvts.getRawParameterValue ("attack")->load();
    p.decay           = apvts.getRawParameterValue ("decay")->load();
    p.sustain         = apvts.getRawParameterValue ("sustain")->load();
    p.release         = apvts.getRawParameterValue ("release")->load();
    p.filterCutoff    = apvts.getRawParameterValue ("filterCutoff")->load();
    p.filterResonance = apvts.getRawParameterValue ("filterResonance")->load();
    p.superSawDetune  = apvts.getRawParameterValue ("superSawDetune")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->updateParams (p);
}

void SynthPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Push unison settings to the synthesiser (take effect on the next note-on)
    synth.numUnisonVoices       = juce::roundToInt (apvts.getRawParameterValue ("unisonVoices")->load());
    synth.unisonDetuneSemitones = apvts.getRawParameterValue ("unisonDetune")->load();

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

    // Waveform: 0=Sine 1=Saw 2=Square 3=Triangle 4=SuperSaw
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "waveform", "Waveform", 0, 4, 0));

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

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "superSawDetune", "SuperSaw Detune",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "unisonVoices", "Unison Voices",
        juce::NormalisableRange<float> (1.0f, 4.0f, 1.0f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "unisonDetune", "Unison Detune",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.1f));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SynthPluginAudioProcessor();
}
