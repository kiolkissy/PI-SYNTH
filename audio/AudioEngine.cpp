#include "audio/AudioEngine.h"

#include <RtAudio.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace synth {

struct AudioEngine::Impl {
    RtAudio rtAudio;
    RenderCallback renderCallback;
    std::vector<float> monoScratch;
};

namespace {

// Prefers a real (non-HDMI) output device -- i.e. a USB audio interface --
// over the Pi's HDMI outputs, since the Pi 5 has no analog audio jack.
unsigned int pickOutputDevice(RtAudio& rtAudio) {
    const auto ids = rtAudio.getDeviceIds();
    unsigned int fallback = 0;
    bool haveFallback = false;

    for (unsigned int id : ids) {
        const RtAudio::DeviceInfo info = rtAudio.getDeviceInfo(id);
        if (info.outputChannels == 0) continue;

        if (!haveFallback) {
            fallback = id;
            haveFallback = true;
        }
        if (info.isDefaultOutput) fallback = id;

        const bool looksLikeHdmi =
            info.name.find("HDMI") != std::string::npos ||
            info.name.find("hdmi") != std::string::npos;
        if (!looksLikeHdmi) {
            std::fprintf(stderr, "AudioEngine: selected output device '%s'\n", info.name.c_str());
            return id;
        }
    }

    if (haveFallback) {
        const RtAudio::DeviceInfo info = rtAudio.getDeviceInfo(fallback);
        std::fprintf(stderr,
                     "AudioEngine: no non-HDMI output device found, falling back to '%s'\n",
                     info.name.c_str());
    }
    return fallback;
}

int rtCallback(void* outputBuffer, void* /*inputBuffer*/, unsigned int numFrames,
               double /*streamTime*/, RtAudioStreamStatus status, void* userData) {
    if (status) {
        std::fprintf(stderr, "AudioEngine: stream underflow/overflow (status=%u)\n",
                     static_cast<unsigned int>(status));
    }

    auto* impl = static_cast<AudioEngine::Impl*>(userData);
    if (impl->monoScratch.size() < numFrames) impl->monoScratch.resize(numFrames);

    if (impl->renderCallback) {
        impl->renderCallback(impl->monoScratch.data(), numFrames);
    } else {
        std::fill(impl->monoScratch.begin(), impl->monoScratch.begin() + numFrames, 0.0f);
    }

    auto* out = static_cast<float*>(outputBuffer);
    for (unsigned int i = 0; i < numFrames; ++i) {
        out[2 * i] = impl->monoScratch[i];
        out[2 * i + 1] = impl->monoScratch[i];
    }
    return 0;
}

} // namespace

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() { stop(); }

bool AudioEngine::start(RenderCallback callback, unsigned int sampleRate,
                         unsigned int bufferFrames) {
    impl_->renderCallback = std::move(callback);

    const unsigned int deviceId = pickOutputDevice(impl_->rtAudio);
    if (deviceId == 0 && impl_->rtAudio.getDeviceIds().empty()) {
        std::fprintf(stderr, "AudioEngine: no audio devices found\n");
        return false;
    }

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = deviceId;
    outputParams.nChannels = 2;
    outputParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.streamName = "synth";

    unsigned int actualBufferFrames = bufferFrames;
    RtAudioErrorType err = impl_->rtAudio.openStream(
        &outputParams, nullptr, RTAUDIO_FLOAT32, sampleRate, &actualBufferFrames, &rtCallback,
        impl_.get(), &options);
    if (err != RTAUDIO_NO_ERROR) {
        std::fprintf(stderr, "AudioEngine: openStream failed\n");
        return false;
    }

    sampleRate_ = sampleRate;

    err = impl_->rtAudio.startStream();
    if (err != RTAUDIO_NO_ERROR) {
        std::fprintf(stderr, "AudioEngine: startStream failed\n");
        return false;
    }

    std::fprintf(stderr, "AudioEngine: started at %u Hz, buffer %u frames\n", sampleRate,
                 actualBufferFrames);
    return true;
}

void AudioEngine::stop() {
    if (impl_->rtAudio.isStreamRunning()) impl_->rtAudio.stopStream();
    if (impl_->rtAudio.isStreamOpen()) impl_->rtAudio.closeStream();
}

} // namespace synth
