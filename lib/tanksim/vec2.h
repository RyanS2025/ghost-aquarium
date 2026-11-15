#pragma once
// 2D float vector for tank physics, in pixels. Header-only, standard library only (ADR 0006).

#include <cmath>

namespace tank {

struct Vec2 {
  float x = 0, y = 0;

  constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
  constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
  constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
  constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
  Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
  Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }

  constexpr float dot(Vec2 o) const { return x * o.x + y * o.y; }
  float length() const { return std::sqrt(x * x + y * y); }

  // Unit vector in the same direction. Returns {0,0} for (near-)zero vectors instead of NaN,
  // so a fish that's stopped or sitting on another fish never poisons the simulation.
  Vec2 normalized() const {
    float l = length();
    return l < 1e-6f ? Vec2{} : Vec2{x / l, y / l};
  }

  // Same direction, length capped at maxLen.
  Vec2 limited(float maxLen) const {
    float l = length();
    return l > maxLen && l > 0.f ? Vec2{x / l * maxLen, y / l * maxLen} : *this;
  }
};

}  // namespace tank
