#pragma once

#include <atomic>
#include <array>
#include <cstdint>

namespace synth {

// v1's fixed set of controllable parameters. The web UI sends
// {"param": <id>, "value": <float>} messages; this is the wire contract
// between it and VoiceManager::applyParamChange.
enum class ParamId : uint32_t {
    FilterCutoffOffset = 0, // Hz, added on top of the per-note base cutoff
    FilterResonance = 1,    // Q, ~0.5-10
    AmpAttack = 2,          // seconds
    AmpDecay = 3,           // seconds
    AmpSustain = 4,         // 0-1
    AmpRelease = 5,         // seconds
    FilterEnvAttack = 6,    // seconds
    FilterEnvDecay = 7,     // seconds
    FilterEnvSustain = 8,   // 0-1
    FilterEnvRelease = 9,   // seconds
    LfoRate = 10,           // Hz
    LfoToCutoffAmount = 11, // Hz
    LfoToPitchAmount = 12,  // semitones
    EnvToCutoffAmount = 13, // Hz

    // Osc A (id offset 20-24) / Osc B (id offset 30-34). Waveform values are
    // encoded as 0=Sine, 1=Saw, 2=Square, 3=Triangle, matching engine::Waveform.
    OscASemitone = 20,      // semitones, transposes Osc A relative to the note
    OscAUnisonVoices = 21,  // integer count, 1-7
    OscAUnisonDetune = 22,  // cents, spread across the unison voices
    OscALevel = 23,         // 0-1
    OscAWaveform = 24,      // waveform enum, see above

    OscBSemitone = 30,
    OscBUnisonVoices = 31,
    OscBUnisonDetune = 32,
    OscBLevel = 33,
    OscBWaveform = 34,

    // Sub oscillator: fixed one octave below the played note, Sine or
    // Square only (the classic sub-bass shapes), plus a broadband noise
    // generator level -- both mixed in alongside Osc A/B.
    SubOscLevel = 40,    // 0-1
    SubOscWaveform = 41, // 0=Sine, 2=Square (reuses engine::Waveform values)
    NoiseLevel = 42,     // 0-1

    // FX chain, applied post-mix (after VoiceManager sums all voices).
    DistortionDrive = 50, // 1-20, how hard the signal is pushed into tanh()
    DistortionMix = 51,   // 0-1, dry/wet
    DelayTimeMs = 52,     // 0-2000 ms
    DelayFeedback = 53,   // 0-0.95
    DelayMix = 54,        // 0-1, dry/wet

    // Voicing: mono vs poly, and glide/portamento time for mono legato.
    VoicingMode = 60, // 0=Poly, 1=Mono
    GlideTime = 61,   // seconds, 0-2 (0 = instant retune, no glide)

    // Generic mod matrix: 2 user-assignable slots, each routing one
    // ModSource to one ModDestination with an amount. Source/destination
    // are sent as small integer enums (see engine::ModSource/ModDestination);
    // VoiceManager re-routes the ModMatrix whenever source or destination
    // changes for a slot.
    ModSlot1Source = 70,
    ModSlot1Dest = 71,
    ModSlot1Amount = 72,
    ModSlot2Source = 80,
    ModSlot2Dest = 81,
    ModSlot2Amount = 82,
};

// A parameter change pushed from the control (WebSocket) thread and
// consumed by the audio callback thread at the top of each block.
struct ParamChange {
    ParamId paramId = ParamId::FilterCutoffOffset;
    float value = 0.0f;
};

// Single-producer/single-consumer lock-free ring buffer. The audio thread
// must never block, so this uses only atomics -- no mutexes, no allocation
// on the hot path.
class ParamQueue {
public:
    static constexpr size_t kCapacity = 256; // power of two

    bool push(const ParamChange& change);
    bool pop(ParamChange& outChange);

private:
    std::array<ParamChange, kCapacity> buffer_{};
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
};

} // namespace synth
