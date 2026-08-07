#include "engine/Filter.h"

#include <algorithm>
#include <cmath>

namespace synth {

void Filter::setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
void Filter::setMode(FilterMode mode) { mode_ = mode; }
void Filter::setCutoff(double hz) { cutoff_ = hz; }
void Filter::setResonance(double q) { resonance_ = q; }
void Filter::reset() {
    low_ = 0.0;
    band_ = 0.0;
}

// Chamberlin state-variable filter. Simple and cheap; stable as long as
// `f` stays comfortably below Nyquist, which the clamp below guarantees.
float Filter::process(float input) {
    const double maxCutoff = sampleRate_ * 0.45;
    const double cutoff = std::clamp(cutoff_, 20.0, maxCutoff);
    const double f = 2.0 * std::sin(M_PI * cutoff / sampleRate_);
    const double q = 1.0 / std::max(resonance_, 0.5);

    const double high = input - low_ - q * band_;
    band_ += f * high;
    low_ += f * band_;

    switch (mode_) {
        case FilterMode::LowPass:
            return static_cast<float>(low_);
        case FilterMode::HighPass:
            return static_cast<float>(high);
        case FilterMode::BandPass:
            return static_cast<float>(band_);
    }
    return static_cast<float>(low_);
}

} // namespace synth
