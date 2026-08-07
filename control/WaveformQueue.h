#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace synth {

// A short capture of raw mono audio samples, taken straight from the audio
// callback, so a UI (or anything else) can visualize what the engine is
// actually outputting -- handy for confirming the synth is producing sound
// even with no speaker/audio interface attached to hear it.
struct WaveformSnapshot {
    static constexpr size_t kMaxSamples = 512;
    std::array<float, kMaxSamples> samples{};
    size_t count = 0;
};

// Single-producer/single-consumer lock-free ring buffer, mirroring
// ParamQueue/NoteQueue's design. The audio thread pushes one snapshot per
// captured block and never blocks; the control thread drains it at its own
// pace to broadcast over the WebSocket.
class WaveformQueue {
public:
    static constexpr size_t kCapacity = 8;

    bool push(const WaveformSnapshot& snapshot);
    bool pop(WaveformSnapshot& outSnapshot);

private:
    std::array<WaveformSnapshot, kCapacity> buffer_{};
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
};

} // namespace synth
