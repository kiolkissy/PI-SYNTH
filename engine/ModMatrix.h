#pragma once

#include <array>
#include <cstddef>

namespace synth {

// Generic mod sources/destinations. v1's UI only exposes a handful of routes,
// but the routing structure itself supports any source -> any destination
// from day one so v3 (full mod matrix UI) is additive, not a rearchitecture.
enum class ModSource {
    Lfo1,
    Env2,
    AmpEnv, // the per-voice amp envelope (Env1), exposed as a mod source too
    Count // sentinel, keep last
};

enum class ModDestination {
    Pitch,
    FilterCutoff,
    OscALevel,
    OscBLevel,
    SubOscLevel,
    Count // sentinel, keep last
};

struct ModRoute {
    ModSource source;
    ModDestination destination;
    float amount = 0.0f; // -1..1
    bool enabled = false;
};

class ModMatrix {
public:
    static constexpr std::size_t kMaxRoutes = 32;

    // Adds or updates a route for (source, destination). Returns false if the
    // route table is full and this would be a new entry.
    bool setRoute(ModSource source, ModDestination destination, float amount);
    void clearRoute(ModSource source, ModDestination destination);

    // Called once per audio block with the current value of each source;
    // returns the summed modulation for a destination.
    void setSourceValue(ModSource source, float value);
    float evaluate(ModDestination destination) const;

private:
    std::array<ModRoute, kMaxRoutes> routes_{};
    std::size_t routeCount_ = 0;
    std::array<float, static_cast<std::size_t>(ModSource::Count)> sourceValues_{};
};

} // namespace synth
