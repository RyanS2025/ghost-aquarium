#pragma once
// Fixed-timestep accumulator (ADR 0005): returns how many whole steps to run; after a stall,
// runs at most maxSteps and drops the rest so fish never teleport.

namespace tank {

class FixedStepper {
 public:
  FixedStepper(float stepSec, int maxSteps) : step_(stepSec), max_(maxSteps) {}

  int advance(float elapsedSec) {
    acc_ += elapsedSec;
    int n = static_cast<int>(acc_ / step_);
    if (n > max_) {
      n = max_;
      acc_ = 0.f;
    } else {
      acc_ -= n * step_;
    }
    return n;
  }

 private:
  float step_;
  int max_;
  float acc_ = 0.f;
};

}  // namespace tank
