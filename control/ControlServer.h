#pragma once

#include <functional>
#include <memory>

#include "control/NoteQueue.h"
#include "control/ParamQueue.h"
#include "control/WaveformQueue.h"

namespace synth {

// WebSocket control server: receives JSON parameter changes and note events
// from the web UI, pushes them onto queues for the audio thread to consume,
// and broadcasts periodic waveform snapshots back out (see WaveformQueue) so
// clients can visualize the engine's actual output. Runs its own thread;
// never touches the audio callback directly.
class ControlServer {
public:
    ControlServer();
    ~ControlServer();

    bool start(int port, ParamQueue& outboundParamsToAudioThread,
               NoteQueue& outboundNotesToAudioThread,
               WaveformQueue& inboundWaveformFromAudioThread);
    void stop();

    // Broadcasts a note-on/off event to every connected client, so the
    // browser's onscreen keyboard can highlight notes played from real MIDI
    // hardware too (not just its own onscreen presses). Safe to call from
    // any thread -- typically the MIDI input callback thread, not the
    // realtime audio thread.
    void broadcastNoteEvent(int note, bool noteOn, float velocity);

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace synth
