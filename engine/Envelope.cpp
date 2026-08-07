#include "engine/Envelope.h"

#include <algorithm>

namespace synth {

void Envelope::setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

void Envelope::setADSR(double attackSec, double decaySec, double sustainLevel, double releaseSec) {
    attackSec_ = std::max(attackSec, 0.0001);
    decaySec_ = std::max(decaySec, 0.0001);
    sustainLevel_ = std::clamp(sustainLevel, 0.0, 1.0);
    releaseSec_ = std::max(releaseSec, 0.0001);
}

void Envelope::setAttack(double attackSec) { attackSec_ = std::max(attackSec, 0.0001); }
void Envelope::setDecay(double decaySec) { decaySec_ = std::max(decaySec, 0.0001); }
void Envelope::setSustain(double sustainLevel) { sustainLevel_ = std::clamp(sustainLevel, 0.0, 1.0); }
void Envelope::setRelease(double releaseSec) { releaseSec_ = std::max(releaseSec, 0.0001); }

void Envelope::noteOn() { stage_ = Stage::Attack; }
void Envelope::noteOff() {
    if (stage_ != Stage::Idle) stage_ = Stage::Release;
}
bool Envelope::isActive() const { return stage_ != Stage::Idle; }

float Envelope::nextSample() {
    switch (stage_) {
        case Stage::Idle:
            level_ = 0.0f;
            break;

        case Stage::Attack: {
            const float step = static_cast<float>(1.0 / (attackSec_ * sampleRate_));
            level_ += step;
            if (level_ >= 1.0f) {
                level_ = 1.0f;
                stage_ = Stage::Decay;
            }
            break;
        }

        case Stage::Decay: {
            const float step = static_cast<float>((1.0 - sustainLevel_) / (decaySec_ * sampleRate_));
            level_ -= step;
            if (level_ <= static_cast<float>(sustainLevel_)) {
                level_ = static_cast<float>(sustainLevel_);
                stage_ = Stage::Sustain;
            }
            break;
        }

        case Stage::Sustain:
            level_ = static_cast<float>(sustainLevel_);
            break;

        case Stage::Release: {
            const float step = static_cast<float>(sustainLevel_ / (releaseSec_ * sampleRate_));
            level_ -= step;
            if (level_ <= 0.0f) {
                level_ = 0.0f;
                stage_ = Stage::Idle;
            }
            break;
        }
    }

    return level_;
}

} // namespace synth
