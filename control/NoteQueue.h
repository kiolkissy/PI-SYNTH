#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace synth {

// A note event pushed from the control (WebSocket) thread -- either from a
// real MIDI controller relayed through the UI, or the browser's onscreen
// keyboard -- and consumed by the audio callback thread at the top of each
// block, same as ParamQueue.
struct NoteEvent {
    bool noteOn = false; // true = note-on, false = note-off
    int note = 0;        // MIDI note number, 0-127
    float velocity = 1.0f;
};

// Single-producer/single-consumer lock-free ring buffer, mirroring
// ParamQueue's design: the audio thread must never block.
class NoteQueue {
public:
    static constexpr size_t kCapacity = 256; // power of two

    bool push(const NoteEvent& event);
    bool pop(NoteEvent& outEvent);

private:
    std::array<NoteEvent, kCapacity> buffer_{};
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
};

} // namespace synth
