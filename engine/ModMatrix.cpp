#include "engine/ModMatrix.h"

namespace synth {

bool ModMatrix::setRoute(ModSource source, ModDestination destination, float amount) {
    for (auto& route : routes_) {
        if (route.enabled && route.source == source && route.destination == destination) {
            route.amount = amount;
            return true;
        }
    }
    if (routeCount_ >= kMaxRoutes) return false;
    routes_[routeCount_] = ModRoute{source, destination, amount, true};
    ++routeCount_;
    return true;
}

void ModMatrix::clearRoute(ModSource source, ModDestination destination) {
    for (auto& route : routes_) {
        if (route.enabled && route.source == source && route.destination == destination) {
            route.enabled = false;
        }
    }
}

void ModMatrix::setSourceValue(ModSource source, float value) {
    sourceValues_[static_cast<std::size_t>(source)] = value;
}

float ModMatrix::evaluate(ModDestination destination) const {
    float sum = 0.0f;
    for (const auto& route : routes_) {
        if (route.enabled && route.destination == destination) {
            sum += sourceValues_[static_cast<std::size_t>(route.source)] * route.amount;
        }
    }
    return sum;
}

} // namespace synth
