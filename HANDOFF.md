# Pi 5 Synth — Project Handoff

## What this is

A real-time MIDI synthesizer engine running on a Raspberry Pi 5, architecturally
modeled on the Novation Summit (multi-oscillator voice architecture, filter
section, mod matrix, effects). Controlled via a hardware MIDI keyboard and a
browser-based web UI. Code lives at `~/synth` on the Pi.

## Status: v1 complete in software, blocked on hardware

As of the last working session (2026-08-06), the full software stack was
built and verified. The only remaining task is physical integration — no
MIDI keyboard or USB audio interface has been connected to the Pi yet.
Checked again on 2026-08-07: `lsusb` shows only a Dell wireless receiver,
and `aconnect -l` shows only kernel/PipeWire virtual MIDI clients — still
no external MIDI/audio hardware attached.

**Next task for whoever picks this up:** connect a MIDI keyboard and USB
audio interface to the Pi, then run the "play a note, hear it" acceptance
test — verify note-on/note-off round-trips through the engine with
acceptable latency.

## System setup (already done, on the Pi)

- CPU governor pinned to `performance` (persisted via a systemd unit).
- `rtkit-daemon` enabled for real-time audio thread priority.
- `cap_sys_nice` capability granted on the compiled engine binary so it can
  request real-time scheduling without running as root.

## Code layout (`~/synth`)

- `engine/` — C++ audio/DSP engine core: `VoiceManager` with polyphony and
  voice-stealing, oscillator + filter + envelope DSP.
- `midi/` — RtMidi-based MIDI input handling.
- `audio/` — RtAudio-based audio output.
- `control/` — JSON-over-WebSocket control protocol (libwebsockets +
  nlohmann-json) feeding a lock-free parameter queue that's consumed on the
  real-time audio thread. Also includes LFOs and a generic `ModMatrix`.
- `ui/` — React/Vite web UI, dark synth-panel styling. Connects to the
  engine over WebSocket. Both `npm run dev` and production build were
  verified working.
- `tools/` — supporting scripts/utilities.
- `build/` — CMake build output, including an offline test target,
  `build/offline_render`, which renders audio straight to a WAV file from
  `VoiceManager` — useful for verifying DSP correctness without any
  physical hardware attached.
- `CMakeLists.txt` — top-level CMake build config (CMake, RtAudio, RtMidi,
  libwebsockets, nlohmann-json as dependencies).
- `run.sh` — convenience launcher for the engine.
- `main.cpp` — engine entry point.

## Build/run

```bash
cd ~/synth
./run.sh          # convenience launcher for the engine
```

For the UI:
```bash
cd ~/synth/ui
npm run dev        # dev server
# or
npm run build       # production build
```

For offline DSP verification without hardware:
```bash
cd ~/synth/build
./offline_render    # renders audio to WAV directly from VoiceManager
```

## Guidance for continuing

- Continue from this existing structure — don't re-scaffold the project.
- Before assuming hardware integration is still blocked, re-check with
  `lsusb` and `aconnect -l` (or ask the user) — hardware may have been
  connected since this doc was written.
- The core software claim ("v1 complete") has not been independently
  re-verified line-by-line in this handoff — treat it as the state as of
  2026-08-06 and confirm the build still compiles/passes before relying on
  it.
