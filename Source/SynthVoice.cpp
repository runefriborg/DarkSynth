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
    subPhase     = 0.0;
    level        = velocity * 0.8f;

    baseFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    double sr     = getSampleRate();
    phaseDelta    = baseFrequency / sr * juce::MathConstants<double>::twoPi;
    subDelta      = phaseDelta * 0.5;

    // Key-track: set bandpass center to the note's frequency immediately
    // so first block uses the correct frequency even before updateParams runs
    double safeFreq = juce::jlimit (20.0, sr * 0.5 - 10.0, baseFrequency);
    filter.setCutoffFrequency ((float) safeFreq);

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
    filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setCutoffFrequency (80.0f);
    filter.setResonance (3.0f);

    subSamples.resize ((size_t) samplesPerBlock, 0.0f);
    adsrSamples.resize ((size_t) samplesPerBlock, 0.0f);

    isPrepared = true;
}

void SynthVoice::updateParams (const SynthParams& p)
{
    if (getSampleRate() <= 0.0) return;

    waveform  = p.waveform;
    focus     = p.focus;
    drive     = p.drive;
    subBlend  = p.subBlend;

    adsrParams.attack  = p.attack;
    adsrParams.decay   = p.decay;
    adsrParams.sustain = p.sustain;
    adsrParams.release = p.release;
    adsr.setParameters (adsrParams);

    double sr       = getSampleRate();
    double safeFreq = juce::jlimit (20.0, sr * 0.5 - 10.0, baseFrequency);
    filter.setCutoffFrequency ((float) safeFreq);
    filter.setResonance (juce::jlimit (0.1f, 10.0f, focus));
}

float SynthVoice::generateSample() noexcept
{
    float sample = 0.0f;

    switch (waveform)
    {
        case 0: // Pure — clean sine
            sample = (float) std::sin (currentPhase);
            break;

        case 1: // Soft — triangle (gentle odd harmonics)
        {
            double t = currentPhase / juce::MathConstants<double>::twoPi;
            sample = (float) (t < 0.5 ? 4.0 * t - 1.0 : 3.0 - 4.0 * t);
            break;
        }

        case 2: // Warm — 0.85·sin(φ) + 0.15·sin(3φ)
            sample = 0.85f * (float) std::sin (currentPhase)
                   + 0.15f * (float) std::sin (3.0 * currentPhase);
            break;

        case 3: // Punch — square
            sample = currentPhase < juce::MathConstants<double>::pi ? 1.0f : -1.0f;
            break;

        case 4: // Grit — hard-clipped sawtooth
        {
            float saw = (float) (1.0 - currentPhase / juce::MathConstants<double>::pi);
            sample = juce::jlimit (-0.7f, 0.7f, saw) / 0.7f;
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

float SynthVoice::applyDrive (float x) const noexcept
{
    if (drive <= 0.0f) return x;
    float k = 1.0f + drive * 4.0f;
    return std::tanh (x * k) / std::tanh (k);
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                   int startSample, int numSamples)
{
    if (!isPrepared || !isVoiceActive())
        return;

    juce::AudioBuffer<float> temp (outputBuffer.getNumChannels(), numSamples);
    temp.clear();

    bool stillActive = true;

    // Zero out envelope and sub arrays so samples past the break point are silent
    std::fill (adsrSamples.begin(), adsrSamples.begin() + numSamples, 0.0f);
    std::fill (subSamples.begin(),  subSamples.begin()  + numSamples, 0.0f);

    for (int s = 0; s < numSamples; ++s)
    {
        float adsrVal = adsr.getNextSample();
        adsrSamples[(size_t) s] = adsrVal;

        // Main oscillator: waveform → drive → temp buffer (ADSR applied post-filter)
        float raw = applyDrive (generateSample()) * level;
        for (int ch = 0; ch < temp.getNumChannels(); ++ch)
            temp.setSample (ch, s, raw);

        // Sub oscillator: pure sine one octave below, stored separately
        subSamples[(size_t) s] = (float) std::sin (subPhase) * level * adsrVal;
        subPhase += subDelta;
        if (subPhase >= juce::MathConstants<double>::twoPi)
            subPhase -= juce::MathConstants<double>::twoPi;

        if (!adsr.isActive())
        {
            stillActive = false;
            break;
        }
    }

    // Apply key-tracked bandpass filter to main oscillator
    juce::dsp::AudioBlock<float>              block (temp);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    filter.process (ctx);

    // JUCE bandpass peaks at Q × input; compensate so level stays consistent
    float gainComp = 1.0f / juce::jmax (1.0f, focus);

    // Mix filtered main (ADSR applied here, post-filter) + unfiltered sub into output
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float out = temp.getSample (ch, s) * gainComp * adsrSamples[(size_t) s]
                      + subBlend * subSamples[(size_t) s];
            outputBuffer.addSample (ch, startSample + s, out);
        }
    }

    if (!stillActive)
        clearCurrentNote();
}
