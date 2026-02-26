#include "SynthVoice.h"

namespace {
    // JP-8000 measured detune offsets, normalised so the outer pair = ±1.0
    // (from Adam Szabo, "How to Emulate the Super Saw", 2010)
    constexpr double kSuperSawDetuneRatios[7] = {
        -1.00000, -0.57166, -0.17730,
         0.0,
         0.18102,  0.56516,  0.97688
    };
    // Maximum spread of the outer pair at detune = 1.0, in cents
    constexpr double kSuperSawMaxDetuneCents = 50.0;
}

SynthVoice::SynthVoice() {}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                             juce::SynthesiserSound*, int)
{
    currentPhase  = 0.0;
    level         = velocity * 0.8f;

    // Apply per-voice unison pitch offset (set by UnisonSynthesiser before this call)
    baseFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber)
                    * std::pow (2.0, (double) unisonDetuneOffset / 12.0);
    phaseDelta    = baseFrequency / getSampleRate() * juce::MathConstants<double>::twoPi;

    // Spread the 7 SuperSaw oscillators evenly across the cycle to avoid
    // a phase-coherent click at note-on
    for (int i = 0; i < kNumSuperSawOscs; ++i)
        superSawPhases[i] = (double) i / kNumSuperSawOscs
                            * juce::MathConstants<double>::twoPi;

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
    waveform      = p.waveform;
    superSawDetune = p.superSawDetune;

    // Recompute per-oscillator phase deltas from the current base frequency and detune
    double sr = getSampleRate();
    if (sr > 0.0)
    {
        for (int i = 0; i < kNumSuperSawOscs; ++i)
        {
            double cents = kSuperSawDetuneRatios[i] * superSawDetune * kSuperSawMaxDetuneCents;
            double freq  = baseFrequency * std::pow (2.0, cents / 1200.0);
            superSawDeltas[i] = freq / sr * juce::MathConstants<double>::twoPi;
        }
    }

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

        case 4: // SuperSaw — 7 detuned sawtooth oscillators (JP-8000 model)
        {
            // Center oscillator (index 3) gets 2× weight to strengthen the fundamental
            // and produce a darker, thicker tone. Total weight = 6×1 + 1×2 = 8.
            float sum = 0.0f;
            for (int i = 0; i < kNumSuperSawOscs; ++i)
            {
                float s = (float) (1.0 - superSawPhases[i] / juce::MathConstants<double>::pi);
                sum += (i == 3) ? s * 2.0f : s;
                superSawPhases[i] += superSawDeltas[i];
                if (superSawPhases[i] >= juce::MathConstants<double>::twoPi)
                    superSawPhases[i] -= juce::MathConstants<double>::twoPi;
            }
            sample = sum * (1.0f / 8.0f);
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
