// Offline test harness: exercises the real DSP engine (VoiceManager, same
// code path as the live audio callback) without touching any audio
// hardware, and writes the result to a WAV file. Useful for verifying the
// oscillators/filter/envelopes/mod-matrix sound correct before a USB audio
// interface is available to test through.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "engine/VoiceManager.h"

namespace {

void writeWavHeader(FILE* f, uint32_t sampleRate, uint32_t numSamples) {
    const uint32_t byteRate = sampleRate * 2 /*channels*/ * 2 /*bytes/sample*/;
    const uint32_t dataSize = numSamples * 2 * 2;
    const uint32_t riffSize = 36 + dataSize;

    std::fwrite("RIFF", 1, 4, f);
    std::fwrite(&riffSize, 4, 1, f);
    std::fwrite("WAVE", 1, 4, f);

    std::fwrite("fmt ", 1, 4, f);
    const uint32_t fmtSize = 16;
    std::fwrite(&fmtSize, 4, 1, f);
    const uint16_t audioFormat = 1; // PCM
    const uint16_t numChannels = 2;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign = numChannels * bitsPerSample / 8;
    std::fwrite(&audioFormat, 2, 1, f);
    std::fwrite(&numChannels, 2, 1, f);
    std::fwrite(&sampleRate, 4, 1, f);
    std::fwrite(&byteRate, 4, 1, f);
    std::fwrite(&blockAlign, 2, 1, f);
    std::fwrite(&bitsPerSample, 2, 1, f);

    std::fwrite("data", 1, 4, f);
    std::fwrite(&dataSize, 4, 1, f);
}

struct NoteEvent {
    double timeSec;
    int note;
    float velocity; // 0 means note-off
};

} // namespace

int main() {
    const double sampleRate = 48000.0;
    synth::VoiceManager voiceManager;
    voiceManager.setSampleRate(sampleRate);

    // Same C-major arpeggio + held chord used for the hardware-free smoke
    // test, so the WAV can be sanity-checked against the MIDI log.
    const std::vector<NoteEvent> events = {
        {0.00, 60, 0.79}, {0.24, 60, 0.0},
        {0.24, 64, 0.79}, {0.48, 64, 0.0},
        {0.48, 67, 0.79}, {0.72, 67, 0.0},
        {0.72, 72, 0.79}, {0.96, 72, 0.0},
        {1.20, 60, 0.71}, {1.20, 64, 0.71}, {1.20, 67, 0.71},
        {2.20, 60, 0.0}, {2.20, 64, 0.0}, {2.20, 67, 0.0},
    };

    const double totalSec = 3.0;
    const uint32_t numSamples = static_cast<uint32_t>(totalSec * sampleRate);

    FILE* f = std::fopen("/tmp/synth_offline_test.wav", "wb");
    if (!f) {
        std::fprintf(stderr, "failed to open output file\n");
        return 1;
    }
    writeWavHeader(f, static_cast<uint32_t>(sampleRate), numSamples);

    size_t nextEvent = 0;
    for (uint32_t i = 0; i < numSamples; ++i) {
        const double t = i / sampleRate;
        while (nextEvent < events.size() && events[nextEvent].timeSec <= t) {
            const auto& e = events[nextEvent];
            if (e.velocity > 0.0f) {
                voiceManager.noteOn(e.note, e.velocity);
            } else {
                voiceManager.noteOff(e.note);
            }
            ++nextEvent;
        }

        float sample = voiceManager.nextSample();
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        const int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
        std::fwrite(&pcm, 2, 1, f); // left
        std::fwrite(&pcm, 2, 1, f); // right
    }

    std::fclose(f);
    std::fprintf(stderr, "wrote /tmp/synth_offline_test.wav (%.1fs)\n", totalSec);
    return 0;
}
