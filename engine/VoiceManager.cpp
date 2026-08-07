#include "engine/VoiceManager.h"

#include <algorithm>
#include <cmath>

namespace synth {

VoiceManager::VoiceManager() {
    lfo_.setRate(4.5);
    // v1's fixed mod routes. The matrix itself supports arbitrary
    // source/destination pairs -- these are just the ones the v1 UI exposes.
    modMatrix_.setRoute(ModSource::Lfo1, ModDestination::Pitch, 0.06f);       // subtle vibrato
    modMatrix_.setRoute(ModSource::Lfo1, ModDestination::FilterCutoff, 150.0f);
    modMatrix_.setRoute(ModSource::Env2, ModDestination::FilterCutoff, 3500.0f);
}

void VoiceManager::setSampleRate(double sampleRate) {
    for (auto& voice : voices_) voice.setSampleRate(sampleRate);
    lfo_.setSampleRate(sampleRate);
}

void VoiceManager::noteOn(int midiNote, float velocity) {
    if (monoMode_) {
        // Remove any existing entry for this note (retriggered key) then
        // push it as the most recently held.
        heldNotes_.erase(std::remove_if(heldNotes_.begin(), heldNotes_.end(),
                                         [midiNote](const auto& n) { return n.first == midiNote; }),
                          heldNotes_.end());
        heldNotes_.emplace_back(midiNote, velocity);

        Voice& voice = voices_[0];
        if (voice.isActive()) {
            voice.retune(midiNote, velocity); // legato: glide, keep envelopes running
        } else {
            voice.setStartOrder(nextVoiceOrder_++);
            voice.noteOn(midiNote, velocity);
        }
        return;
    }

    // Prefer a free voice; if all are busy, steal the oldest active one
    // (lowest startOrder) rather than always voice 0, so a run of notes
    // doesn't keep clobbering whichever note happens to sit in slot 0.
    Voice* oldest = nullptr;
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            voice.setStartOrder(nextVoiceOrder_++);
            voice.noteOn(midiNote, velocity);
            return;
        }
        if (!oldest || voice.startOrder() < oldest->startOrder()) {
            oldest = &voice;
        }
    }
    oldest->setStartOrder(nextVoiceOrder_++);
    oldest->noteOn(midiNote, velocity);
}

void VoiceManager::noteOff(int midiNote) {
    if (monoMode_) {
        heldNotes_.erase(std::remove_if(heldNotes_.begin(), heldNotes_.end(),
                                         [midiNote](const auto& n) { return n.first == midiNote; }),
                          heldNotes_.end());
        Voice& voice = voices_[0];
        if (!voice.isActive() || voice.currentNote() != midiNote) return;
        if (heldNotes_.empty()) {
            voice.noteOff(); // last held key released -- let the envelope release
        } else {
            const auto& [note, vel] = heldNotes_.back();
            voice.retune(note, vel); // fall back to the previous held note (legato)
        }
        return;
    }

    for (auto& voice : voices_) {
        if (voice.isActive() && voice.currentNote() == midiNote) {
            voice.noteOff();
        }
    }
}

void VoiceManager::setMonoMode(bool mono) {
    monoMode_ = mono;
    heldNotes_.clear();
}

void VoiceManager::setGlideTime(double seconds) {
    glideTimeSeconds_ = seconds;
    for (auto& voice : voices_) voice.setGlideTime(seconds);
}

void VoiceManager::applyModSlot(int slotIndex) {
    ModSlot& slot = modSlots_[slotIndex];
    if (slot.everApplied) {
        modMatrix_.clearRoute(slot.appliedSource, slot.appliedDestination);
    }
    if (slot.amount != 0.0f) {
        // The UI's amount knob is a normalized -1..1 depth; scale it to a
        // sensible range per destination unit (semitones/Hz/gain) here so
        // one knob feels musically useful regardless of what it's routed to.
        float scaledAmount = slot.amount;
        switch (slot.destination) {
            case ModDestination::Pitch:
                scaledAmount *= 12.0f; // up to +-1 octave
                break;
            case ModDestination::FilterCutoff:
                scaledAmount *= 4000.0f; // up to +-4kHz
                break;
            case ModDestination::OscALevel:
            case ModDestination::OscBLevel:
            case ModDestination::SubOscLevel:
                // amount is already a -1..1 gain offset, used as-is.
                break;
            case ModDestination::Count:
                break;
        }
        modMatrix_.setRoute(slot.source, slot.destination, scaledAmount);
    }
    slot.appliedSource = slot.source;
    slot.appliedDestination = slot.destination;
    slot.everApplied = true;
}

void VoiceManager::setModSlotSource(int slotIndex, int source) {
    modSlots_[slotIndex].source = static_cast<ModSource>(source);
    applyModSlot(slotIndex);
}
void VoiceManager::setModSlotDestination(int slotIndex, int destination) {
    modSlots_[slotIndex].destination = static_cast<ModDestination>(destination);
    applyModSlot(slotIndex);
}
void VoiceManager::setModSlotAmount(int slotIndex, float amount) {
    modSlots_[slotIndex].amount = amount;
    applyModSlot(slotIndex);
}

void VoiceManager::setLfoRate(double hz) { lfo_.setRate(hz); }
void VoiceManager::setLfoToCutoffAmount(float hz) {
    modMatrix_.setRoute(ModSource::Lfo1, ModDestination::FilterCutoff, hz);
}
void VoiceManager::setLfoToPitchAmount(float semitones) {
    modMatrix_.setRoute(ModSource::Lfo1, ModDestination::Pitch, semitones);
}
void VoiceManager::setEnvToCutoffAmount(float hz) {
    modMatrix_.setRoute(ModSource::Env2, ModDestination::FilterCutoff, hz);
}

void VoiceManager::setFilterCutoffOffset(float hz) {
    for (auto& voice : voices_) voice.setFilterCutoffOffset(hz);
}
void VoiceManager::setFilterResonance(float q) {
    for (auto& voice : voices_) voice.setFilterResonance(q);
}
void VoiceManager::setAmpAttack(double s) { for (auto& voice : voices_) voice.setAmpAttack(s); }
void VoiceManager::setAmpDecay(double s) { for (auto& voice : voices_) voice.setAmpDecay(s); }
void VoiceManager::setAmpSustain(double s) { for (auto& voice : voices_) voice.setAmpSustain(s); }
void VoiceManager::setAmpRelease(double s) { for (auto& voice : voices_) voice.setAmpRelease(s); }
void VoiceManager::setFilterEnvAttack(double s) { for (auto& voice : voices_) voice.setFilterEnvAttack(s); }
void VoiceManager::setFilterEnvDecay(double s) { for (auto& voice : voices_) voice.setFilterEnvDecay(s); }
void VoiceManager::setFilterEnvSustain(double s) { for (auto& voice : voices_) voice.setFilterEnvSustain(s); }
void VoiceManager::setFilterEnvRelease(double s) { for (auto& voice : voices_) voice.setFilterEnvRelease(s); }

void VoiceManager::setOscWaveform(int oscIndex, Waveform waveform) {
    for (auto& voice : voices_) voice.setOscWaveform(oscIndex, waveform);
}
void VoiceManager::setOscSemitoneOffset(int oscIndex, float semitones) {
    for (auto& voice : voices_) voice.setOscSemitoneOffset(oscIndex, semitones);
}
void VoiceManager::setOscUnisonVoices(int oscIndex, int voices) {
    for (auto& voice : voices_) voice.setOscUnisonVoices(oscIndex, voices);
}
void VoiceManager::setOscUnisonDetuneCents(int oscIndex, float cents) {
    for (auto& voice : voices_) voice.setOscUnisonDetuneCents(oscIndex, cents);
}
void VoiceManager::setOscLevel(int oscIndex, float level) {
    for (auto& voice : voices_) voice.setOscLevel(oscIndex, level);
}

void VoiceManager::setSubOscWaveform(Waveform waveform) {
    for (auto& voice : voices_) voice.setSubOscWaveform(waveform);
}
void VoiceManager::setSubOscLevel(float level) {
    for (auto& voice : voices_) voice.setSubOscLevel(level);
}
void VoiceManager::setNoiseLevel(float level) {
    for (auto& voice : voices_) voice.setNoiseLevel(level);
}

float VoiceManager::nextSample() {
    modMatrix_.setSourceValue(ModSource::Lfo1, lfo_.nextSample());

    float sum = 0.0f;
    int activeCount = 0;
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            sum += voice.nextSample(modMatrix_);
            ++activeCount;
        }
    }
    // Cheap headroom management so polyphony doesn't clip as more voices stack.
    if (activeCount > 1) sum /= std::sqrt(static_cast<float>(activeCount));
    return sum;
}

} // namespace synth
