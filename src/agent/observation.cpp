#include "life_worlds/agent/observation.hpp"

#include <algorithm>

namespace life_worlds {
namespace agent {

// Discretize distance in cells into one of four vision buckets.
int distance_bucket(int cells) {
  if (cells <= 1) {
    return 0;
  }
  if (cells <= 4) {
    return 1;
  }
  if (cells <= 10) {
    return 2;
  }
  return 3;
}

// Discretize current energy as a fraction of energy_max into four buckets.
int energy_bucket(int energy, int energy_max) {
  if (energy_max <= 0) {
    return 0;
  }
  const double ratio =
      static_cast<double>(std::max(0, energy)) /
      static_cast<double>(energy_max);
  if (ratio >= 0.75) {
    return 3;
  }
  if (ratio >= 0.50) {
    return 2;
  }
  if (ratio >= 0.25) {
    return 1;
  }
  return 0;
}

}  // namespace agent
}  // namespace life_worlds
