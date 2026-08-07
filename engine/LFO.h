#pragma once

namespace synth {

class LFO {
public:
    void setSampleRate(double sampleRate);
    void setRate(double hz);
    void reset();

    // Returns the next value in [-1, 1].
    float nextSample();

private:
    double sampleRate_ = 48000.0;
    double rateHz_ = 2.0;
    double phase_ = 0.0;
};

} // namespace synth
