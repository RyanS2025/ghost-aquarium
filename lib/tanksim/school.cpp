#include "school.h"

namespace tank {

Vec2 steerSchool(const Fish& self, const Fish* fish, int count, const Tuning& t, const FxTuning& fx) {
  // TODO(human): implement body-aware schooling (see school.h and test/test_school).
  // Until then fish swim without schooling; everything else in the tank still works.
  (void)self; (void)fish; (void)count; (void)t; (void)fx;
  return {};
}

}  // namespace tank
