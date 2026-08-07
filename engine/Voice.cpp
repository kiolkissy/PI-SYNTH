#include "engine/Voice.h"

#include <algorithm>
#include <cmath>

namespace synth {

namespace {
double midiNoteToHz(int note) {
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}
} // namespace

void Voice::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    osc1_.setSampleRate(sampleRate);
    osc2_.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);

    osc1_.setWaveform(Waveform::Saw);
    osc1_.setLevel(0.45f);
    osc2_.setWaveform(Waveform::Square);
    osc2_.setSemitoneOffset(0.0f);
    osc2_.setLevel(0.45f);
    subOsc_.setSampleRate(sampleRate);
    subOsc_.setWaveform(Waveform::Sine);
    filter_.setMode(FilterMode::LowPass);
    filter_.setResonance(0.9);

    ampEnv_.setADSR(0.005, 0.15, 0.7, 0.3);
    filterEnv_.setADSR(0.01, 0.25, 0.3, 0.4);
}

void Voice::noteOn(int midiNote, float velocity) {
    midiNote_ = midiNote;
    velocity_ = velocity;
    active_ = true;

    targetFrequencyHz_ = midiNoteToHz(midiNote);
    baseFrequencyHz_ = targetFrequencyHz_; // instant retune on a fresh note-on
    baseCutoffHz_ = 800.0 + velocity * 800.0; // brighter with harder velocity

    osc1_.reset();
    osc2_.reset();
    subOsc_.reset();
    filter_.reset();
    ampEnv_.noteOn();
    filterEnv_.noteOn();
}

void Voice::retune(int midiNote, float velocity) {
    // Mono-mode legato: change pitch/velocity target without retriggering
    // envelopes or resetting oscillator phase -- baseFrequencyHz_ glides
    // toward the new target in nextSample() instead of jumping.
    midiNote_ = midiNote;
    velocity_ = velocity;
    active_ = true;
    targetFrequencyHz_ = midiNoteToHz(midiNote);
    baseCutoffHz_ = 800.0 + velocity * 800.0;
}

void Voice::setGlideTime(double seconds) {
    glideTimeSeconds_ = seconds;
    if (seconds <= 0.0) {
        glideCoeff_ = 0.0; // 0 means "snap instantly" (see nextSample)
    } else {
        // Standard one-pole time constant: after `seconds`, ~63% of the way there.
        glideCoeff_ = std::exp(-1.0 / (seconds * sampleRate_));
    }
}

void Voice::noteOff() {
    ampEnv_.noteOff();
    filterEnv_.noteOff();
}

bool Voice::isActive() const { return active_; }

float Voice::nextSample(ModMatrix& modMatrix) {
    if (!active_) return 0.0f;

    if (glideCoeff_ <= 0.0) {
        baseFrequencyHz_ = targetFrequencyHz_;
    } else {
        baseFrequencyHz_ = targetFrequencyHz_ + (baseFrequencyHz_ - targetFrequencyHz_) * glideCoeff_;
    }

    const float ampEnvValue = ampEnv_.nextSample();
    modMatrix.setSourceValue(ModSource::Env2, filterEnv_.nextSample());
    modMatrix.setSourceValue(ModSource::AmpEnv, ampEnvValue);
    const float cutoffModHz = modMatrix.evaluate(ModDestination::FilterCutoff);
    const float pitchModSemitones = modMatrix.evaluate(ModDestination::Pitch);
    // Level destinations are gain multipliers around 1.0 (mod amount is an
    // offset, clamped so a route can silence but not invert an oscillator).
    const float oscAGain = std::clamp(1.0f + modMatrix.evaluate(ModDestination::OscALevel), 0.0f, 2.0f);
    const float oscBGain = std::clamp(1.0f + modMatrix.evaluate(ModDestination::OscBLevel), 0.0f, 2.0f);
    const float subGain = std::clamp(1.0f + modMatrix.evaluate(ModDestination::SubOscLevel), 0.0f, 2.0f);

    const double freq = baseFrequencyHz_ * std::pow(2.0, pitchModSemitones / 12.0);

    filter_.setCutoff(baseCutoffHz_ + cutoffModHz + filterCutoffOffset_);

    const float noise = (static_cast<float>(noiseRng_()) / static_cast<float>(noiseRng_.max())) * 2.0f - 1.0f;
    subOsc_.setFrequency(freq * 0.5); // one octave below the played note
    const float mixed = osc1_.nextSample(freq) * oscAGain + osc2_.nextSample(freq) * oscBGain +
                         subOscLevel_ * subGain * subOsc_.nextSample() + noiseLevel_ * noise;

    const float filtered = filter_.process(mixed);
    const float amp = ampEnvValue * velocity_;

    if (!ampEnv_.isActive()) {
        active_ = false;
    }

    return filtered * amp;
}

} // namespace synth
