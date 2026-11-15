#include "school.h"

namespace tank {

Vec2 steerSchool(const Fish& self, const Fish* fish, int count, const Tuning& t, const FxTuning& fx) {
  const int L = static_cast<int>(self.layer);
  const float sepR = t.separationRadius * kBodySpace[L];  // bigger fish need more room
  const float crossR = 0.7f * sepR;                       // lighter spacing across depth layers
  const float ms = t.maxSpeed[L];

  Vec2 sep{}, avgVel{}, center{};
  int n = 0;
  for (int i = 0; i < count; ++i) {
    const Fish& o = fish[i];
    if (&o == &self || o.away || o.airborne) continue;

    // Separation is measured to the NEAREST POINT OF THE NEIGHBOUR'S BODY, not its head, so long
    // fish keep real clearance (the prototype skipped this when the head was far: tails overlapped).
    Vec2 q = o.pos;
    float best = (o.pos - self.pos).length();
    for (int j = 0; j < o.bodyN; ++j) {
      const float d = (o.body[j] - self.pos).length();
      if (d < best) { best = d; q = o.body[j]; }
    }
    const Vec2 away = self.pos - q;
    const float d = away.length();

    if (o.layer != self.layer) {
      if (d < crossR && d > 1e-4f) sep += away * (fx.layerSpacing * (crossR - d) / crossR / d);
      continue;
    }
    if (d < sepR && d > 1e-4f) sep += away * ((sepR - d) / sepR / d);  // d > 1e-4: coincident fish stay finite
    if ((o.pos - self.pos).length() > t.neighborRadius) continue;     // alignment/cohesion: heads in range
    ++n;
    avgVel += o.vel;
    center += o.pos;
  }

  // Reynolds "seek": steer from the current velocity toward full speed along a direction.
  auto seek = [&](Vec2 dir) {
    const Vec2 u = dir.normalized();
    return u.length() == 0.f ? Vec2{} : u * ms - self.vel;
  };
  Vec2 r = seek(sep) * t.separationWeight;
  if (n > 0) {
    r += seek(avgVel / static_cast<float>(n)) * t.alignmentWeight;
    r += seek(center / static_cast<float>(n) - self.pos) * t.cohesionWeight;
  }
  return r;
}

}  // namespace tank
