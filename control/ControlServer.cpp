#include "control/ControlServer.h"

#include <libwebsockets.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace synth {

struct ControlServer::Impl {
    lws_context* context = nullptr;
    std::thread serviceThread;
    std::atomic<bool> running{false};
    ParamQueue* paramQueue = nullptr;
    NoteQueue* noteQueue = nullptr;
    WaveformQueue* waveformQueue = nullptr;

    // Connected clients, so a freshly-arrived waveform snapshot can be
    // broadcast to everyone rather than just whoever sent the last message.
    std::mutex clientsMutex;
    std::vector<lws*> clients;

    // Latest waveform snapshot, pre-serialized to JSON once per broadcast
    // rather than once per client.
    std::mutex waveformMutex;
    std::string latestWaveformJson;

    // Outbound note-on/off events (from real MIDI hardware, so the browser's
    // onscreen keyboard can highlight external-keyboard playing too). Unlike
    // the waveform snapshot, none of these can be dropped as "stale" -- each
    // discrete on/off must reach the client -- so they're queued per-client
    // rather than collapsed to a single "latest" value.
    std::mutex noteEchoMutex;
    std::unordered_map<lws*, std::deque<std::string>> pendingNoteEchoes;
};

namespace {

// Wire protocol: the web UI sends one of two JSON shapes per WS text frame:
//   {"param": <int ParamId>, "value": <float>}                  -- knob changes
//   {"type": "noteOn", "note": <int>, "velocity": <float 0-1>}  -- onscreen keyboard
//   {"type": "noteOff", "note": <int>}
// The server sends this shape back out to every connected client:
//   {"type": "waveform", "samples": [<float>, ...]}             -- live scope data
// Runs on the control thread, so exceptions/allocation here are fine -- only
// the audio thread must avoid them.
void handleMessage(ParamQueue* paramQueue, NoteQueue* noteQueue, const char* data, size_t len) {
    if (!paramQueue && !noteQueue) return;
    try {
        const auto json = nlohmann::json::parse(data, data + len);

        if (json.contains("type")) {
            if (!noteQueue) return;
            const auto type = json.at("type").get<std::string>();
            NoteEvent event;
            event.note = json.at("note").get<int>();
            if (type == "noteOn") {
                event.noteOn = true;
                event.velocity = json.value("velocity", 1.0f);
            } else if (type == "noteOff") {
                event.noteOn = false;
            } else {
                std::fprintf(stderr, "ControlServer: unknown message type '%s'\n", type.c_str());
                return;
            }
            if (!noteQueue->push(event)) {
                std::fprintf(stderr, "ControlServer: note queue full, dropping event\n");
            }
            return;
        }

        if (!paramQueue) return;
        const auto paramId = static_cast<ParamId>(json.at("param").get<uint32_t>());
        const float value = json.at("value").get<float>();
        if (!paramQueue->push(ParamChange{paramId, value})) {
            std::fprintf(stderr, "ControlServer: param queue full, dropping change\n");
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ControlServer: failed to parse message: %s\n", e.what());
    }
}

std::string buildWaveformJson(const WaveformSnapshot& snapshot) {
    nlohmann::json j;
    j["type"] = "waveform";
    auto& samples = j["samples"] = nlohmann::json::array();
    for (size_t i = 0; i < snapshot.count; ++i) {
        samples.push_back(snapshot.samples[i]);
    }
    return j.dump();
}

int wsCallback(lws* wsi, lws_callback_reasons reason, void* /*user*/, void* in, size_t len) {
    auto* impl = static_cast<ControlServer::Impl*>(lws_context_user(lws_get_context(wsi)));
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            std::fprintf(stderr, "ControlServer: client connected\n");
            std::lock_guard<std::mutex> lock(impl->clientsMutex);
            impl->clients.push_back(wsi);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            std::fprintf(stderr, "ControlServer: client disconnected\n");
            std::lock_guard<std::mutex> lock(impl->clientsMutex);
            impl->clients.erase(std::remove(impl->clients.begin(), impl->clients.end(), wsi),
                                 impl->clients.end());
            std::lock_guard<std::mutex> noteLock(impl->noteEchoMutex);
            impl->pendingNoteEchoes.erase(wsi);
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            handleMessage(impl->paramQueue, impl->noteQueue, static_cast<const char*>(in), len);
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            // Discrete note echoes take priority over the waveform (which
            // is fine to skip a frame of), since every on/off must arrive.
            std::string noteJson;
            {
                std::lock_guard<std::mutex> lock(impl->noteEchoMutex);
                auto it = impl->pendingNoteEchoes.find(wsi);
                if (it != impl->pendingNoteEchoes.end() && !it->second.empty()) {
                    noteJson = std::move(it->second.front());
                    it->second.pop_front();
                    if (!it->second.empty()) lws_callback_on_writable(wsi);
                }
            }
            if (!noteJson.empty()) {
                std::vector<unsigned char> buf(LWS_PRE + noteJson.size());
                std::memcpy(buf.data() + LWS_PRE, noteJson.data(), noteJson.size());
                lws_write(wsi, buf.data() + LWS_PRE, noteJson.size(), LWS_WRITE_TEXT);
                break;
            }

            std::string json;
            {
                std::lock_guard<std::mutex> lock(impl->waveformMutex);
                json = impl->latestWaveformJson;
            }
            if (!json.empty()) {
                std::vector<unsigned char> buf(LWS_PRE + json.size());
                std::memcpy(buf.data() + LWS_PRE, json.data(), json.size());
                lws_write(wsi, buf.data() + LWS_PRE, json.size(), LWS_WRITE_TEXT);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

const lws_protocols kProtocols[] = {
    {"synth-control", wsCallback, 0, 4096, 0, nullptr, 0},
    LWS_PROTOCOL_LIST_TERM,
};

} // namespace

ControlServer::ControlServer() : impl_(std::make_unique<Impl>()) {}
ControlServer::~ControlServer() { stop(); }

bool ControlServer::start(int port, ParamQueue& outboundParamsToAudioThread,
                           NoteQueue& outboundNotesToAudioThread,
                           WaveformQueue& inboundWaveformFromAudioThread) {
    impl_->paramQueue = &outboundParamsToAudioThread;
    impl_->noteQueue = &outboundNotesToAudioThread;
    impl_->waveformQueue = &inboundWaveformFromAudioThread;

    lws_context_creation_info info{};
    info.port = port;
    info.protocols = kProtocols;
    info.gid = -1;
    info.uid = -1;
    info.user = impl_.get();

    impl_->context = lws_create_context(&info);
    if (!impl_->context) {
        std::fprintf(stderr, "ControlServer: lws_create_context failed\n");
        return false;
    }

    impl_->running = true;
    impl_->serviceThread = std::thread([this] {
        while (impl_->running.load()) {
            lws_service(impl_->context, 20);

            // Drain to the newest available waveform snapshot -- for a live
            // scope display only the most recent capture matters, so any
            // older ones queued up are simply stale and skipped.
            WaveformSnapshot snapshot;
            bool haveSnapshot = false;
            while (impl_->waveformQueue->pop(snapshot)) haveSnapshot = true;

            if (haveSnapshot) {
                {
                    std::lock_guard<std::mutex> lock(impl_->waveformMutex);
                    impl_->latestWaveformJson = buildWaveformJson(snapshot);
                }
                std::lock_guard<std::mutex> lock(impl_->clientsMutex);
                for (lws* wsi : impl_->clients) {
                    lws_callback_on_writable(wsi);
                }
            }
        }
    });

    std::fprintf(stderr, "ControlServer: listening on port %d\n", port);
    return true;
}

void ControlServer::broadcastNoteEvent(int note, bool noteOn, float velocity) {
    nlohmann::json j;
    j["type"] = noteOn ? "noteOn" : "noteOff";
    j["note"] = note;
    if (noteOn) j["velocity"] = velocity;
    std::string json = j.dump();

    std::lock_guard<std::mutex> lock(impl_->clientsMutex);
    std::lock_guard<std::mutex> noteLock(impl_->noteEchoMutex);
    for (lws* wsi : impl_->clients) {
        impl_->pendingNoteEchoes[wsi].push_back(json);
        lws_callback_on_writable(wsi);
    }
    if (impl_->context) lws_cancel_service(impl_->context);
}

void ControlServer::stop() {
    if (!impl_->running.exchange(false)) return;
    // lws_service() blocks in poll() for up to its timeout argument; without
    // this it can take much longer than expected to notice `running` went
    // false, since the poll only wakes on socket activity or its timeout.
    if (impl_->context) lws_cancel_service(impl_->context);
    if (impl_->serviceThread.joinable()) impl_->serviceThread.join();
    if (impl_->context) {
        lws_context_destroy(impl_->context);
        impl_->context = nullptr;
    }
}

} // namespace synth
