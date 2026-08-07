#pragma once

#include <functional>
#include <memory>

namespace RtMidiNS { class RtMidiIn; }

namespace synth {

class MidiInput {
public:
    using NoteOnHandler = std::function<void(int note, float velocity)>;
    using NoteOffHandler = std::function<void(int note)>;

    MidiInput();
    ~MidiInput();

    // Opens the first available USB-MIDI input port. Returns false if none found.
    bool openFirstAvailablePort();

    void setNoteOnHandler(NoteOnHandler handler);
    void setNoteOffHandler(NoteOffHandler handler);

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace synth
