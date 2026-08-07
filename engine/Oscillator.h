#pragma once

#include <array>

namespace synth {

enum class Waveform { Sine, Saw, Square, Triangle };

class Oscillator {
public:
    void setSampleRate(double sampleRate);
    void setFrequency(double hz);
    void setWaveform(Waveform waveform);
    void reset();

    // Returns the next sample in [-1, 1].
    float nextSample();

private:
    static constexpr double kLeak = 0.001; // leaky integrator coefficient for Triangle

    double sampleRate_ = 48000.0;
    double frequency_ = 440.0;
    double phase_ = 0.0;
    double triangleIntegrator_ = 0.0;
    Waveform waveform_ = Waveform::Saw;
};

// A single "oscillator slot" as seen in the UI (Osc A / Osc B): a waveform
// plus up to kMaxUnison detuned copies summed together (a classic "unison"
// / "supersaw" style thickening effect), a semitone offset for interval
// tuning relative to the played note, and an overall level for blending
// against the other oscillator.
class OscillatorUnit {
public:
    static constexpr int kMaxUnison = 7;

    void setSampleRate(double sampleRate);
    void setWaveform(Waveform waveform);
    void setSemitoneOffset(float semitones) { semitoneOffset_ = semitones; }
    void setUnisonVoices(int voices);
    void setUnisonDetuneCents(float cents) { unisonDetuneCents_ = cents; }
    void setLevel(float level) { level_ = level; }
    void reset();

    // Renders this oscillator's contribution for one sample, given the
    // voice's current (possibly pitch-modulated) base frequency in Hz.
    float nextSample(double baseFrequencyHz);

private:
    std::array<Oscillator, kMaxUnison> oscs_;
    Waveform waveform_ = Waveform::Saw;
    int unisonVoices_ = 1;
    float unisonDetuneCents_ = 0.0f;
    float semitoneOffset_ = 0.0f;
    float level_ = 0.5f;
};

} // namespace synth
