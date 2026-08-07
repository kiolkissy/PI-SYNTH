#include <atomic>
#include <cstdio>
#include <csignal>
#include <thread>
#include <chrono>
#include <algorithm>

#include "audio/AudioEngine.h"
#include "control/ControlServer.h"
#include "control/NoteQueue.h"
#include "control/ParamQueue.h"
#include "control/WaveformQueue.h"
#include "engine/Effects.h"
#include "engine/VoiceManager.h"
#include "midi/MidiInput.h"

namespace {
std::atomic<bool> g_running{true};
void handleSignal(int) { g_running = false; }
} // namespace

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    synth::VoiceManager voiceManager;
    synth::MidiInput midiInput;
    synth::AudioEngine audioEngine;
    synth::ParamQueue paramQueue;
    synth::NoteQueue noteQueue;
    synth::WaveformQueue waveformQueue;
    synth::ControlServer controlServer;
    synth::Distortion distortion;
    synth::Delay delay;
    delay.setSampleRate(audioEngine.sampleRate());
    delay.setTimeSeconds(0.25f);

    midiInput.setNoteOnHandler([&voiceManager, &controlServer](int note, float velocity) {
        voiceManager.noteOn(note, velocity);
        controlServer.broadcastNoteEvent(note, true, velocity);
        std::fprintf(stderr, "note on  %d vel=%.2f\n", note, velocity);
    });
    midiInput.setNoteOffHandler([&voiceManager, &controlServer](int note) {
        voiceManager.noteOff(note);
        controlServer.broadcastNoteEvent(note, false, 0.0f);
        std::fprintf(stderr, "note off %d\n", note);
    });

    if (!midiInput.openFirstAvailablePort()) {
        std::fprintf(stderr, "main: continuing without MIDI input (none found)\n");
    }

    const bool audioStarted = audioEngine.start(
        [&voiceManager, &paramQueue, &noteQueue, &waveformQueue, &distortion, &delay](float* out, unsigned int numFrames) {
            // Drain param changes once per block -- never blocks, never
            // allocates, safe on the realtime audio thread.
            synth::ParamChange change;
            while (paramQueue.pop(change)) {
                std::fprintf(stderr, "param update: id=%u value=%.3f\n",
                             static_cast<unsigned>(change.paramId), change.value);
                switch (change.paramId) {
                    case synth::ParamId::FilterCutoffOffset:
                        voiceManager.setFilterCutoffOffset(change.value);
                        break;
                    case synth::ParamId::FilterResonance:
                        voiceManager.setFilterResonance(change.value);
                        break;
                    case synth::ParamId::AmpAttack:
                        voiceManager.setAmpAttack(change.value);
                        break;
                    case synth::ParamId::AmpDecay:
                        voiceManager.setAmpDecay(change.value);
                        break;
                    case synth::ParamId::AmpSustain:
                        voiceManager.setAmpSustain(change.value);
                        break;
                    case synth::ParamId::AmpRelease:
                        voiceManager.setAmpRelease(change.value);
                        break;
                    case synth::ParamId::FilterEnvAttack:
                        voiceManager.setFilterEnvAttack(change.value);
                        break;
                    case synth::ParamId::FilterEnvDecay:
                        voiceManager.setFilterEnvDecay(change.value);
                        break;
                    case synth::ParamId::FilterEnvSustain:
                        voiceManager.setFilterEnvSustain(change.value);
                        break;
                    case synth::ParamId::FilterEnvRelease:
                        voiceManager.setFilterEnvRelease(change.value);
                        break;
                    case synth::ParamId::LfoRate:
                        voiceManager.setLfoRate(change.value);
                        break;
                    case synth::ParamId::LfoToCutoffAmount:
                        voiceManager.setLfoToCutoffAmount(change.value);
                        break;
                    case synth::ParamId::LfoToPitchAmount:
                        voiceManager.setLfoToPitchAmount(change.value);
                        break;
                    case synth::ParamId::EnvToCutoffAmount:
                        voiceManager.setEnvToCutoffAmount(change.value);
                        break;
                    case synth::ParamId::OscASemitone:
                        voiceManager.setOscSemitoneOffset(0, change.value);
                        break;
                    case synth::ParamId::OscAUnisonVoices:
                        voiceManager.setOscUnisonVoices(0, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::OscAUnisonDetune:
                        voiceManager.setOscUnisonDetuneCents(0, change.value);
                        break;
                    case synth::ParamId::OscALevel:
                        voiceManager.setOscLevel(0, change.value);
                        break;
                    case synth::ParamId::OscAWaveform:
                        voiceManager.setOscWaveform(0, static_cast<synth::Waveform>(static_cast<int>(change.value)));
                        break;
                    case synth::ParamId::OscBSemitone:
                        voiceManager.setOscSemitoneOffset(1, change.value);
                        break;
                    case synth::ParamId::OscBUnisonVoices:
                        voiceManager.setOscUnisonVoices(1, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::OscBUnisonDetune:
                        voiceManager.setOscUnisonDetuneCents(1, change.value);
                        break;
                    case synth::ParamId::OscBLevel:
                        voiceManager.setOscLevel(1, change.value);
                        break;
                    case synth::ParamId::OscBWaveform:
                        voiceManager.setOscWaveform(1, static_cast<synth::Waveform>(static_cast<int>(change.value)));
                        break;
                    case synth::ParamId::SubOscLevel:
                        voiceManager.setSubOscLevel(change.value);
                        break;
                    case synth::ParamId::SubOscWaveform:
                        voiceManager.setSubOscWaveform(static_cast<synth::Waveform>(static_cast<int>(change.value)));
                        break;
                    case synth::ParamId::NoiseLevel:
                        voiceManager.setNoiseLevel(change.value);
                        break;
                    case synth::ParamId::DistortionDrive:
                        distortion.setDrive(change.value);
                        break;
                    case synth::ParamId::DistortionMix:
                        distortion.setMix(change.value);
                        break;
                    case synth::ParamId::DelayTimeMs:
                        delay.setTimeSeconds(change.value / 1000.0f);
                        break;
                    case synth::ParamId::DelayFeedback:
                        delay.setFeedback(change.value);
                        break;
                    case synth::ParamId::DelayMix:
                        delay.setMix(change.value);
                        break;
                    case synth::ParamId::VoicingMode:
                        voiceManager.setMonoMode(change.value >= 0.5f);
                        break;
                    case synth::ParamId::GlideTime:
                        voiceManager.setGlideTime(change.value);
                        break;
                    case synth::ParamId::ModSlot1Source:
                        voiceManager.setModSlotSource(0, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::ModSlot1Dest:
                        voiceManager.setModSlotDestination(0, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::ModSlot1Amount:
                        voiceManager.setModSlotAmount(0, change.value);
                        break;
                    case synth::ParamId::ModSlot2Source:
                        voiceManager.setModSlotSource(1, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::ModSlot2Dest:
                        voiceManager.setModSlotDestination(1, static_cast<int>(change.value));
                        break;
                    case synth::ParamId::ModSlot2Amount:
                        voiceManager.setModSlotAmount(1, change.value);
                        break;
                }
            }

            // Drain note events from the UI's onscreen keyboard (or any
            // other WS client) the same way -- top of block, never blocks.
            synth::NoteEvent noteEvent;
            while (noteQueue.pop(noteEvent)) {
                if (noteEvent.noteOn) {
                    voiceManager.noteOn(noteEvent.note, noteEvent.velocity);
                    std::fprintf(stderr, "note on  %d vel=%.2f (ws)\n", noteEvent.note, noteEvent.velocity);
                } else {
                    voiceManager.noteOff(noteEvent.note);
                    std::fprintf(stderr, "note off %d (ws)\n", noteEvent.note);
                }
            }

            for (unsigned int i = 0; i < numFrames; ++i) {
                float sample = voiceManager.nextSample();
                sample = distortion.process(sample);
                sample = delay.process(sample);
                out[i] = sample;
            }

            // Push a raw capture of this block to the UI's oscilloscope
            // every few blocks (~30Hz at 256 frames/48kHz) -- enough for a
            // smooth live display without flooding the WebSocket. Dropping
            // the push on a full queue is fine; it's just a visual aid.
            static unsigned int blockCounter = 0;
            constexpr unsigned int kSnapshotStride = 6;
            if (++blockCounter % kSnapshotStride == 0) {
                synth::WaveformSnapshot snapshot;
                snapshot.count = std::min<size_t>(numFrames, synth::WaveformSnapshot::kMaxSamples);
                std::copy_n(out, snapshot.count, snapshot.samples.begin());
                waveformQueue.push(snapshot);
            }
        });

    if (!audioStarted) {
        std::fprintf(stderr, "main: failed to start audio engine, exiting\n");
        return 1;
    }

    voiceManager.setSampleRate(audioEngine.sampleRate());

    if (!controlServer.start(9002, paramQueue, noteQueue, waveformQueue)) {
        std::fprintf(stderr, "main: failed to start control server, exiting\n");
        return 1;
    }

    std::fprintf(stderr, "synth: running. Ctrl+C to quit.\n");
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::fprintf(stderr, "synth: shutting down\n");
    controlServer.stop();
    audioEngine.stop();
    return 0;
}
