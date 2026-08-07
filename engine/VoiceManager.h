#pragma once

#include <array>
#include <utility>
#include <vector>

#include "engine/LFO.h"
#include "engine/ModMatrix.h"
#include "engine/Voice.h"

namespace synth {

class VoiceManager {
public:
    static constexpr int kMaxVoices = 16;

    VoiceManager();

    void setSampleRate(double sampleRate);

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);

    // Fixed v1 mod route amounts, exposed so the control layer can adjust
    // them later without touching ModMatrix's generic plumbing.
    void setLfoRate(double hz);
    void setLfoToCutoffAmount(float hz);
    void setLfoToPitchAmount(float semitones);
    void setEnvToCutoffAmount(float hz);

    // Broadcast to every voice (present and future notes).
    void setFilterCutoffOffset(float hz);
    void setFilterResonance(float q);
    void setAmpAttack(double s);
    void setAmpDecay(double s);
    void setAmpSustain(double s);
    void setAmpRelease(double s);
    void setFilterEnvAttack(double s);
    void setFilterEnvDecay(double s);
    void setFilterEnvSustain(double s);
    void setFilterEnvRelease(double s);

    // Osc A/B controls (oscIndex 0 = A, 1 = B), broadcast to every voice.
    void setOscWaveform(int oscIndex, Waveform waveform);
    void setOscSemitoneOffset(int oscIndex, float semitones);
    void setOscUnisonVoices(int oscIndex, int voices);
    void setOscUnisonDetuneCents(int oscIndex, float cents);
    void setOscLevel(int oscIndex, float level);

    // Sub oscillator / noise controls, broadcast to every voice.
    void setSubOscWaveform(Waveform waveform);
    void setSubOscLevel(float level);
    void setNoiseLevel(float level);

    // Voicing: mono (single voice, legato glide between held notes) vs
    // poly (existing behavior -- each note gets its own voice/envelope).
    void setMonoMode(bool mono);
    void setGlideTime(double seconds);

    // Generic mod matrix slots (2 user-assignable routes). `slotIndex` is 0
    // or 1; source/destination are passed as raw ints matching the
    // ModSource/ModDestination enum order (as sent over the wire protocol).
    void setModSlotSource(int slotIndex, int source);
    void setModSlotDestination(int slotIndex, int destination);
    void setModSlotAmount(int slotIndex, float amount);

    float nextSample();

private:
    void applyModSlot(int slotIndex);

    std::array<Voice, kMaxVoices> voices_;
    ModMatrix modMatrix_;
    LFO lfo_;
    uint64_t nextVoiceOrder_ = 0;

    bool monoMode_ = false;
    double glideTimeSeconds_ = 0.0;
    // Stack of currently-held notes (most recent last), used in mono mode
    // for legato behavior: releasing the top note falls back to the next
    // most recently held one instead of cutting off, matching how
    // hardware mono synths behave.
    std::vector<std::pair<int, float>> heldNotes_;

    // 2 user-assignable mod matrix slots. Each slot re-applies its route to
    // modMatrix_ whenever source/destination/amount changes, replacing
    // whatever route it previously occupied (so re-pointing a slot doesn't
    // leave a stale route behind).
    struct ModSlot {
        ModSource source = ModSource::Lfo1;
        ModDestination destination = ModDestination::Pitch;
        float amount = 0.0f;
        // The (source, destination) this slot last wrote to modMatrix_, so
        // applyModSlot can clear the old route before writing the new one
        // when the user re-points a slot to a different source/destination.
        ModSource appliedSource = ModSource::Lfo1;
        ModDestination appliedDestination = ModDestination::Pitch;
        bool everApplied = false;
    };
    std::array<ModSlot, 2> modSlots_{};
};

} // namespace synth
