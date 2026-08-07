#pragma once

#include <cstdint>
#include <random>

#include "engine/Envelope.h"
#include "engine/Filter.h"
#include "engine/ModMatrix.h"
#include "engine/Oscillator.h"

namespace synth {

class Voice {
public:
    void setSampleRate(double sampleRate);

    void noteOn(int midiNote, float velocity);
    void noteOff();
    bool isActive() const;
    int currentNote() const { return midiNote_; }

    // Monotonically increasing counter stamped by VoiceManager on noteOn, so
    // it can identify the oldest active voice when it needs to steal one.
    void setStartOrder(uint64_t order) { startOrder_ = order; }
    uint64_t startOrder() const { return startOrder_; }

    // Renders one sample. `modMatrix` supplies global modulation sources
    // (e.g. the shared LFO, set once per sample by VoiceManager); this call
    // also writes this voice's own filter-envelope value into it before
    // evaluating destinations, since Env2 is per-voice.
    float nextSample(ModMatrix& modMatrix);

    // Shared-parameter setters, applied identically to every voice by
    // VoiceManager when a control message arrives. Safe to call live (mid
    // note) since they only change stepping/coefficients, not stage/level.
    void setFilterCutoffOffset(float hz) { filterCutoffOffset_ = hz; }
    void setFilterResonance(float q) { filter_.setResonance(q); }
    void setAmpAttack(double s) { ampEnv_.setAttack(s); }
    void setAmpDecay(double s) { ampEnv_.setDecay(s); }
    void setAmpSustain(double s) { ampEnv_.setSustain(s); }
    void setAmpRelease(double s) { ampEnv_.setRelease(s); }
    void setFilterEnvAttack(double s) { filterEnv_.setAttack(s); }
    void setFilterEnvDecay(double s) { filterEnv_.setDecay(s); }
    void setFilterEnvSustain(double s) { filterEnv_.setSustain(s); }
    void setFilterEnvRelease(double s) { filterEnv_.setRelease(s); }

    // Per-oscillator (A/B) live controls, applied identically to every
    // voice by VoiceManager. `oscIndex` is 0 for Osc A, 1 for Osc B.
    void setOscWaveform(int oscIndex, Waveform waveform) { osc(oscIndex).setWaveform(waveform); }
    void setOscSemitoneOffset(int oscIndex, float semitones) { osc(oscIndex).setSemitoneOffset(semitones); }
    void setOscUnisonVoices(int oscIndex, int voices) { osc(oscIndex).setUnisonVoices(voices); }
    void setOscUnisonDetuneCents(int oscIndex, float cents) { osc(oscIndex).setUnisonDetuneCents(cents); }
    void setOscLevel(int oscIndex, float level) { osc(oscIndex).setLevel(level); }

    // Sub oscillator (fixed one octave below the note, Sine/Square only)
    // and broadband noise generator -- both simple fixed-role sources that
    // add low-end weight and texture alongside Osc A/B.
    void setSubOscWaveform(Waveform waveform) { subOsc_.setWaveform(waveform); }
    void setSubOscLevel(float level) { subOscLevel_ = level; }
    void setNoiseLevel(float level) { noiseLevel_ = level; }

    // Glide/portamento: when > 0, base frequency slides exponentially
    // toward the target note's frequency instead of jumping instantly.
    // Used for both regular noteOn (retune from whatever frequency the
    // voice is currently at) and mono-mode legato (see `retune`).
    void setGlideTime(double seconds);

    // Mono-mode legato: changes the played note/velocity without
    // retriggering the envelopes (they keep running), sliding the pitch
    // via glide instead. Used by VoiceManager when a new key is pressed
    // while another is already held in mono mode.
    void retune(int midiNote, float velocity);

private:
    OscillatorUnit& osc(int oscIndex) { return oscIndex == 0 ? osc1_ : osc2_; }

    OscillatorUnit osc1_;
    OscillatorUnit osc2_;
    Oscillator subOsc_;
    float subOscLevel_ = 0.0f;
    float noiseLevel_ = 0.02f;
    Filter filter_;
    Envelope ampEnv_;
    Envelope filterEnv_;
    std::minstd_rand noiseRng_{std::random_device{}()};

    double baseFrequencyHz_ = 440.0;
    double targetFrequencyHz_ = 440.0;
    double glideCoeff_ = 0.0; // per-sample exponential coefficient toward target
    double glideTimeSeconds_ = 0.0;
    double sampleRate_ = 48000.0;
    double baseCutoffHz_ = 1200.0;
    float filterCutoffOffset_ = 0.0f;

    int midiNote_ = -1;
    float velocity_ = 0.0f;
    bool active_ = false;
    uint64_t startOrder_ = 0;
};

} // namespace synth
