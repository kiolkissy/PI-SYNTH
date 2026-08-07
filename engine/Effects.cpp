#include "engine/Effects.h"

#include <algorithm>
#include <cmath>

namespace synth {

float Distortion::process(float input) const {
    if (mix_ <= 0.0f) return input;
    const float driven = std::tanh(input * drive_) / std::tanh(drive_);
    return input * (1.0f - mix_) + driven * mix_;
}

void Delay::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    // Up to 2 seconds of delay time at this sample rate.
    buffer_.assign(static_cast<size_t>(sampleRate * 2.0) + 1, 0.0f);
    writeIndex_ = 0;
}

void Delay::setTimeSeconds(float seconds) {
    if (buffer_.empty()) return;
    seconds = std::clamp(seconds, 0.0f, 2.0f);
    delaySamples_ = static_cast<size_t>(seconds * sampleRate_);
    if (delaySamples_ >= buffer_.size()) delaySamples_ = buffer_.size() - 1;
}

void Delay::setFeedback(float feedback) {
    feedback_ = std::clamp(feedback, 0.0f, 0.95f);
}

void Delay::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writeIndex_ = 0;
}

float Delay::process(float input) {
    if (buffer_.empty() || mix_ <= 0.0f) return input;

    const size_t readIndex = (writeIndex_ + buffer_.size() - delaySamples_) % buffer_.size();
    const float delayed = buffer_[readIndex];

    buffer_[writeIndex_] = input + delayed * feedback_;
    writeIndex_ = (writeIndex_ + 1) % buffer_.size();

    return input * (1.0f - mix_) + delayed * mix_;
}

} // namespace synth
