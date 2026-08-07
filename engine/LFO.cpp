#include "engine/LFO.h"

#include <cmath>

namespace synth {

void LFO::setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
void LFO::setRate(double hz) { rateHz_ = hz; }
void LFO::reset() { phase_ = 0.0; }

float LFO::nextSample() {
    const float out = static_cast<float>(std::sin(2.0 * M_PI * phase_));
    phase_ += rateHz_ / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return out;
}

} // namespace synth
