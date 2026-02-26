#pragma once
#include <JuceHeader.h>

// Parameters passed from processor to each voice each block
struct SynthParams
{
    int   waveform        = 0;     // 0=Sine 1=Saw 2=Square 3=Triangle
    float attack          = 0.1f;
    float decay           = 0.1f;
    float sustain         = 0.8f;
    float release         = 0.5f;
    float filterCutoff    = 2000.0f;
    float filterResonance = 0.7f;
};

// ---- Sound (trivial – every note plays every sound) ----
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

// ---- Voice ----
class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int)  override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels);
    void updateParams  (const SynthParams& p);

private:
    float generateSample() noexcept;

    double currentPhase = 0.0;
    double phaseDelta   = 0.0;
    float  level        = 0.0f;
    int    waveform     = 0;

    juce::ADSR                               adsr;
    juce::ADSR::Parameters                   adsrParams;
    juce::dsp::StateVariableTPTFilter<float> filter;

    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthVoice)
};
