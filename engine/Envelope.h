#pragma once

namespace synth {

class Envelope {
public:
    void setSampleRate(double sampleRate);
    void setADSR(double attackSec, double decaySec, double sustainLevel, double releaseSec);
    void setAttack(double attackSec);
    void setDecay(double decaySec);
    void setSustain(double sustainLevel);
    void setRelease(double releaseSec);

    void noteOn();
    void noteOff();
    bool isActive() const;

    float nextSample();

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    double sampleRate_ = 48000.0;
    double attackSec_ = 0.01;
    double decaySec_ = 0.1;
    double sustainLevel_ = 0.7;
    double releaseSec_ = 0.2;

    Stage stage_ = Stage::Idle;
    float level_ = 0.0f;
};

} // namespace synth
