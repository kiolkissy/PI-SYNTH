#pragma once

namespace synth {

enum class FilterMode { LowPass, HighPass, BandPass };

class Filter {
public:
    void setSampleRate(double sampleRate);
    void setMode(FilterMode mode);
    void setCutoff(double hz);
    void setResonance(double q);
    void reset();

    float process(float input);

private:
    double sampleRate_ = 48000.0;
    FilterMode mode_ = FilterMode::LowPass;
    double cutoff_ = 1000.0;
    double resonance_ = 0.7;
    double low_ = 0.0;
    double band_ = 0.0;
};

} // namespace synth
