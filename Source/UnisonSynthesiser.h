#pragma once
#include <JuceHeader.h>
#include "SynthVoice.h"

// Extends juce::Synthesiser to start numUnisonVoices voices per note-on,
// each with a pitch offset spread symmetrically across ±unisonDetuneSemitones.
class UnisonSynthesiser : public juce::Synthesiser
{
public:
    int   numUnisonVoices      = 1;
    float unisonDetuneSemitones = 0.1f;

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        const juce::ScopedLock sl (lock);

        for (auto* sound : sounds)
        {
            if (!sound->appliesToNote (midiNoteNumber) || !sound->appliesToChannel (midiChannel))
                continue;

            // Stop any existing voices on this note (matches base-class behaviour)
            for (auto* v : voices)
                if (v->getCurrentlyPlayingNote() == midiNoteNumber && v->isPlayingChannel (midiChannel))
                    stopVoice (v, 1.0f, true);

            // Start numUnisonVoices voices spread across ±unisonDetuneSemitones
            for (int i = 0; i < numUnisonVoices; ++i)
            {
                float offset = 0.0f;
                if (numUnisonVoices > 1)
                    offset = ((float) i / (float) (numUnisonVoices - 1) - 0.5f)
                             * 2.0f * unisonDetuneSemitones;

                auto* voice = findFreeVoice (sound, midiChannel, midiNoteNumber,
                                             isNoteStealingEnabled());
                if (auto* sv = dynamic_cast<SynthVoice*> (voice))
                    sv->setUnisonDetuneOffset (offset);

                startVoice (voice, sound, midiChannel, midiNoteNumber, velocity);
            }
            break;
        }
    }
};
