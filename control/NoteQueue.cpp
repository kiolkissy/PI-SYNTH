#include "control/NoteQueue.h"

namespace synth {

bool NoteQueue::push(const NoteEvent& event) {
    const size_t writeIdx = writeIndex_.load(std::memory_order_relaxed);
    const size_t nextWriteIdx = (writeIdx + 1) % kCapacity;
    if (nextWriteIdx == readIndex_.load(std::memory_order_acquire)) {
        return false; // full
    }
    buffer_[writeIdx] = event;
    writeIndex_.store(nextWriteIdx, std::memory_order_release);
    return true;
}

bool NoteQueue::pop(NoteEvent& outEvent) {
    const size_t readIdx = readIndex_.load(std::memory_order_relaxed);
    if (readIdx == writeIndex_.load(std::memory_order_acquire)) {
        return false; // empty
    }
    outEvent = buffer_[readIdx];
    readIndex_.store((readIdx + 1) % kCapacity, std::memory_order_release);
    return true;
}

} // namespace synth
