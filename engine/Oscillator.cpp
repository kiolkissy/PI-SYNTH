#include "engine/Oscillator.h"

#include <cmath>

namespace synth {

namespace {

// Polynomial band-limited step correction, applied at discontinuities so
// saw/square don't alias. `t` is phase in [0,1), `dt` is phase increment
// per sample (i.e. frequency / sampleRate).
double polyBlep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

} // namespace

void Oscillator::setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
void Oscillator::setFrequency(double hz) { frequency_ = hz; }
void Oscillator::setWaveform(Waveform waveform) { waveform_ = waveform; }
void Oscillator::reset() { phase_ = 0.0; }

float Oscillator::nextSample() {
    const double dt = frequency_ / sampleRate_;
    double out = 0.0;

    switch (waveform_) {
        case Waveform::Sine:
            out = std::sin(2.0 * M_PI * phase_);
            break;

        case Waveform::Saw:
            out = 2.0 * phase_ - 1.0;
            out -= polyBlep(phase_, dt);
            break;

        case Waveform::Square: {
            out = phase_ < 0.5 ? 1.0 : -1.0;
            out += polyBlep(phase_, dt);
            double shifted = phase_ + 0.5;
            if (shifted >= 1.0) shifted -= 1.0;
            out -= polyBlep(shifted, dt);
            break;
        }

        case Waveform::Triangle: {
            // Leaky-integrated band-limited square, standard polyBLEP trick.
            double square = phase_ < 0.5 ? 1.0 : -1.0;
            square += polyBlep(phase_, dt);
            double shifted = phase_ + 0.5;
            if (shifted >= 1.0) shifted -= 1.0;
            square -= polyBlep(shifted, dt);

            triangleIntegrator_ = (1.0 - kLeak) * triangleIntegrator_ + dt * square;
            out = triangleIntegrator_ * 4.0; // rescale to roughly [-1, 1]
            break;
        }
    }

    phase_ += dt;
    if (phase_ >= 1.0) phase_ -= 1.0;

    return static_cast<float>(out);
}

void OscillatorUnit::setSampleRate(double sampleRate) {
    for (auto& osc : oscs_) osc.setSampleRate(sampleRate);
}

void OscillatorUnit::setWaveform(Waveform waveform) {
    waveform_ = waveform;
    for (auto& osc : oscs_) osc.setWaveform(waveform);
}

void OscillatorUnit::setUnisonVoices(int voices) {
    unisonVoices_ = voices < 1 ? 1 : (voices > kMaxUnison ? kMaxUnison : voices);
}

void OscillatorUnit::reset() {
    for (auto& osc : oscs_) osc.reset();
}

float OscillatorUnit::nextSample(double baseFrequencyHz) {
    const double semitoneRatio = std::pow(2.0, semitoneOffset_ / 12.0);
    const double centeredFreq = baseFrequencyHz * semitoneRatio;

    if (unisonVoices_ <= 1) {
        oscs_[0].setFrequency(centeredFreq);
        return oscs_[0].nextSample() * level_;
    }

    // Spread unison voices evenly across +-unisonDetuneCents_/2, so the
    // center of the spread always sits on pitch regardless of voice count.
    float sum = 0.0f;
    for (int i = 0; i < unisonVoices_; ++i) {
        const float t = unisonVoices_ == 1 ? 0.0f
                                            : (static_cast<float>(i) / (unisonVoices_ - 1)) * 2.0f - 1.0f;
        const double detuneCents = t * (unisonDetuneCents_ * 0.5);
        const double detuneRatio = std::pow(2.0, detuneCents / 1200.0);
        oscs_[i].setFrequency(centeredFreq * detuneRatio);
        sum += oscs_[i].nextSample();
    }
    // Normalize by sqrt(voices) rather than voices so more unison voices
    // still sound louder/thicker, not just averaged down to the same level.
    return (sum / std::sqrt(static_cast<float>(unisonVoices_))) * level_;
}

} // namespace synth
