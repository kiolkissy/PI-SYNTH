#pragma once

#include <functional>
#include <memory>

namespace synth {

class AudioEngine {
public:
    // Called once per audio block to fill `numFrames` mono samples into `out`.
    using RenderCallback = std::function<void(float* out, unsigned int numFrames)>;

    AudioEngine();
    ~AudioEngine();

    double sampleRate() const { return sampleRate_; }

    // Opens the default (or first non-HDMI) output device and starts the
    // audio callback. Returns false on failure.
    bool start(RenderCallback callback, unsigned int sampleRate = 48000,
               unsigned int bufferFrames = 256);
    void stop();

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
    double sampleRate_ = 48000.0;
};

} // namespace synth
