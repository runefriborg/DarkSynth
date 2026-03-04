#pragma once
#include <JuceHeader.h>
#include <vector>

// Parameters passed from processor to each voice each block
struct SynthParams
{
    int   waveform = 0;      // 0=Pure 1=Soft 2=Warm 3=Punch 4=Grit
    float attack   = 0.01f;
    float decay    = 0.30f;
    float sustain  = 0.70f;
    float release  = 0.20f;
    float focus    = 3.0f;  // bandpass Q (1.0–8.0)
    float drive    = 0.0f;  // tanh saturation (0.0–1.0)
    float subBlend = 0.0f;  // sub-octave blend post-filter (0.0–1.0)
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
    float applyDrive (float x) const noexcept;

    double currentPhase  = 0.0;
    double phaseDelta    = 0.0;
    double subPhase      = 0.0;
    double subDelta      = 0.0;
    float  level         = 0.0f;
    int    waveform      = 0;
    double baseFrequency = 440.0;

    float focus    = 3.0f;
    float drive    = 0.0f;
    float subBlend = 0.0f;

    std::vector<float> subSamples;
    std::vector<float> adsrSamples;

    juce::ADSR                               adsr;
    juce::ADSR::Parameters                   adsrParams;
    juce::dsp::StateVariableTPTFilter<float> filter;

    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthVoice)
};
