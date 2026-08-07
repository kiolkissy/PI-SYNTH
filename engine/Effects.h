#pragma once

#include <vector>

namespace synth {

// Simple tanh-based waveshaper "drive" distortion, applied to the final
// mixed signal (post-VoiceManager). `drive` controls how hard the signal
// is pushed into the tanh curve (1 = mostly clean, higher = more clipping);
// output is normalized by tanh(drive) so the effect doesn't just get
// quieter as drive increases. `mix` blends between dry and processed.
class Distortion {
public:
    void setDrive(float drive) { drive_ = drive < 0.01f ? 0.01f : drive; }
    void setMix(float mix) { mix_ = mix; }

    float process(float input) const;

private:
    float drive_ = 1.0f;
    float mix_ = 0.0f;
};

// A basic feedback delay line (single tap), for slap-back/echo effects.
// `timeSeconds` sets the delay length, `feedback` controls how much of the
// delayed signal is fed back in (0-0.95, clamped to avoid runaway buildup),
// and `mix` blends dry vs. wet.
class Delay {
public:
    void setSampleRate(double sampleRate);
    void setTimeSeconds(float seconds);
    void setFeedback(float feedback);
    void setMix(float mix) { mix_ = mix; }
    void reset();

    float process(float input);

private:
    double sampleRate_ = 48000.0;
    std::vector<float> buffer_;
    size_t writeIndex_ = 0;
    size_t delaySamples_ = 0;
    float feedback_ = 0.3f;
    float mix_ = 0.0f;
};

} // namespace synth
