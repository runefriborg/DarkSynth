#include "SynthVoice.h"

SynthVoice::SynthVoice() {}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                             juce::SynthesiserSound*, int)
{
    currentPhase = 0.0;
    level        = velocity * 0.8f;

    auto freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phaseDelta  = freq / getSampleRate() * juce::MathConstants<double>::twoPi;

    adsr.noteOn();
    filter.reset();
}

void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
        adsr.noteOff();
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels)
{
    adsr.setSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = (juce::uint32) numChannels;

    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency (2000.0f);
    filter.setResonance (0.7f);

    isPrepared = true;
}

void SynthVoice::updateParams (const SynthParams& p)
{
    waveform = p.waveform;

    adsrParams.attack  = p.attack;
    adsrParams.decay   = p.decay;
    adsrParams.sustain = p.sustain;
    adsrParams.release = p.release;
    adsr.setParameters (adsrParams);

    filter.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, p.filterCutoff));
    filter.setResonance       (juce::jlimit (0.1f,  10.0f,    p.filterResonance));
}

float SynthVoice::generateSample() noexcept
{
    float sample = 0.0f;

    switch (waveform)
    {
        case 0: // Sine
            sample = (float) std::sin (currentPhase);
            break;

        case 1: // Sawtooth  (phase 0→2π maps to 1→-1)
            sample = (float) (1.0 - currentPhase / juce::MathConstants<double>::pi);
            break;

        case 2: // Square
            sample = currentPhase < juce::MathConstants<double>::pi ? 1.0f : -1.0f;
            break;

        case 3: // Triangle
        {
            double t = currentPhase / juce::MathConstants<double>::twoPi; // 0..1
            sample = (float) (t < 0.5 ? 4.0 * t - 1.0 : 3.0 - 4.0 * t);
            break;
        }

        default:
            sample = (float) std::sin (currentPhase);
    }

    currentPhase += phaseDelta;
    if (currentPhase >= juce::MathConstants<double>::twoPi)
        currentPhase -= juce::MathConstants<double>::twoPi;

    return sample;
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                   int startSample, int numSamples)
{
    if (!isPrepared || !isVoiceActive())
        return;

    // Render into a temporary buffer so we can apply the per-voice filter
    juce::AudioBuffer<float> temp (outputBuffer.getNumChannels(), numSamples);
    temp.clear();

    bool stillActive = true;

    for (int s = 0; s < numSamples; ++s)
    {
        float adsrVal = adsr.getNextSample();
        float raw     = generateSample() * level * adsrVal;

        for (int ch = 0; ch < temp.getNumChannels(); ++ch)
            temp.setSample (ch, s, raw);

        if (!adsr.isActive())
        {
            stillActive = false;
            break;
        }
    }

    // Apply low-pass filter
    juce::dsp::AudioBlock<float>            block (temp);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    filter.process (ctx);

    // Mix into the host buffer
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom (ch, startSample, temp, ch, 0, numSamples);

    if (!stillActive)
        clearCurrentNote();
}
