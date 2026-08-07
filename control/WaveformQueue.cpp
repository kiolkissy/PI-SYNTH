#include "control/WaveformQueue.h"

namespace synth {

bool WaveformQueue::push(const WaveformSnapshot& snapshot) {
    const size_t writeIdx = writeIndex_.load(std::memory_order_relaxed);
    const size_t nextWriteIdx = (writeIdx + 1) % kCapacity;
    if (nextWriteIdx == readIndex_.load(std::memory_order_acquire)) {
        return false; // full
    }
    buffer_[writeIdx] = snapshot;
    writeIndex_.store(nextWriteIdx, std::memory_order_release);
    return true;
}

bool WaveformQueue::pop(WaveformSnapshot& outSnapshot) {
    const size_t readIdx = readIndex_.load(std::memory_order_relaxed);
    if (readIdx == writeIndex_.load(std::memory_order_acquire)) {
        return false; // empty
    }
    outSnapshot = buffer_[readIdx];
    readIndex_.store((readIdx + 1) % kCapacity, std::memory_order_release);
    return true;
}

} // namespace synth
