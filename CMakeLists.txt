cmake_minimum_required(VERSION 3.18)
project(synth CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
add_compile_options(-Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RTAUDIO REQUIRED IMPORTED_TARGET rtaudio)
pkg_check_modules(RTMIDI REQUIRED IMPORTED_TARGET rtmidi)

find_package(nlohmann_json REQUIRED)
find_package(libwebsockets REQUIRED)
find_package(Threads REQUIRED)

add_executable(synth
  main.cpp
  engine/Oscillator.cpp
  engine/Filter.cpp
  engine/Envelope.cpp
  engine/LFO.cpp
  engine/ModMatrix.cpp
  engine/Voice.cpp
  engine/VoiceManager.cpp
  engine/Effects.cpp
  midi/MidiInput.cpp
  audio/AudioEngine.cpp
  control/ParamQueue.cpp
  control/NoteQueue.cpp
  control/WaveformQueue.cpp
  control/ControlServer.cpp
)

target_include_directories(synth PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(synth PRIVATE
  PkgConfig::RTAUDIO
  PkgConfig::RTMIDI
  nlohmann_json::nlohmann_json
  websockets
  Threads::Threads
)

# Hardware-free DSP smoke test: renders the engine straight to a WAV file,
# no audio/MIDI/network deps needed.
add_executable(offline_render
  tools/offline_render.cpp
  engine/Oscillator.cpp
  engine/Filter.cpp
  engine/Envelope.cpp
  engine/LFO.cpp
  engine/ModMatrix.cpp
  engine/Voice.cpp
  engine/VoiceManager.cpp
  engine/Effects.cpp
)
target_include_directories(offline_render PRIVATE ${CMAKE_SOURCE_DIR})
