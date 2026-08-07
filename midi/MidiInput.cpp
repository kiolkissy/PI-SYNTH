#include "midi/MidiInput.h"

#include <RtMidi.h>

#include <cstdio>
#include <vector>

namespace synth {

struct MidiInput::Impl {
    RtMidiIn rtMidiIn;
    NoteOnHandler noteOnHandler;
    NoteOffHandler noteOffHandler;
};

namespace {

void midiCallback(double /*timeStamp*/, std::vector<unsigned char>* message, void* userData) {
    auto* impl = static_cast<MidiInput::Impl*>(userData);
    if (!message || message->size() < 3) return;

    const unsigned char status = (*message)[0] & 0xF0;
    const unsigned char note = (*message)[1];
    const unsigned char velocity = (*message)[2];

    if (status == 0x90 && velocity > 0) {
        if (impl->noteOnHandler) impl->noteOnHandler(note, velocity / 127.0f);
    } else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
        if (impl->noteOffHandler) impl->noteOffHandler(note);
    }
}

} // namespace

MidiInput::MidiInput() : impl_(std::make_unique<Impl>()) {}
MidiInput::~MidiInput() = default;

bool MidiInput::openFirstAvailablePort() {
    const unsigned int portCount = impl_->rtMidiIn.getPortCount();
    if (portCount == 0) {
        std::fprintf(stderr, "MidiInput: no MIDI input ports found\n");
        return false;
    }

    std::fprintf(stderr, "MidiInput: opening port 0 (%s)\n",
                 impl_->rtMidiIn.getPortName(0).c_str());
    impl_->rtMidiIn.openPort(0);
    impl_->rtMidiIn.setCallback(&midiCallback, impl_.get());
    impl_->rtMidiIn.ignoreTypes(true, true, true); // ignore sysex/timing/sense
    return true;
}

void MidiInput::setNoteOnHandler(NoteOnHandler handler) { impl_->noteOnHandler = std::move(handler); }
void MidiInput::setNoteOffHandler(NoteOffHandler handler) { impl_->noteOffHandler = std::move(handler); }

} // namespace synth
