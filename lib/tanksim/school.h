#pragma once
// Schooling (DESIGN §5, ADR 0009): separation measured to neighbours' BODIES, alignment and
// cohesion with same-layer neighbours, light spacing across depth layers.

#include "fish.h"
#include "tuning.h"
#include "vec2.h"

namespace tank {

// Separation radius per layer: separationRadius × {0.7, 1.0, 1.35} (bigger fish need more room).
constexpr float kBodySpace[kLayerCount] = {0.7f, 1.0f, 1.35f};

// Steering force on `self` from the other fish in `fish[0..count)`.
//  - skips `self` (by address), away fish and airborne fish
//  - same layer: separation from the nearest point of each neighbour's body (f.body[0..bodyN))
//    within sepR = separationRadius × kBodySpace[layer], weighted (sepR − d) / sepR / d;
//    alignment + cohesion over neighbours whose HEADS are within neighborRadius
//  - other layers: separation only, within 0.7·sepR, weighted × fx.layerSpacing
//  - each component is a Reynolds "seek": normalize(dir)·maxSpeed[layer] − self.vel
//    (zero for a zero direction); result = sepW·sep + aliW·ali + cohW·coh
Vec2 steerSchool(const Fish& self, const Fish* fish, int count, const Tuning& t, const FxTuning& fx);

}  // namespace tank
