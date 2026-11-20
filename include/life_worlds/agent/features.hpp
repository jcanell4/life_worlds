#pragma once

#include "life_worlds/agent/observation.hpp"

#include <array>

namespace life_worlds {
namespace agent {

// Per ray: distance one-hot (4) + type one-hot (4) + growth_stage (1).
inline constexpr int kPerRayFeatureCount =
    kDistanceBucketCount + kObjectTypeCount + 1;
// Max over 8 rays + energy one-hot (4).
inline constexpr int kMaxFeatureSize =
    kMaxVisionRays * kPerRayFeatureCount + kEnergyBucketCount;

inline int feature_size(int ray_count) {
  const int rays = (ray_count <= 0) ? 4 : ray_count;
  return rays * kPerRayFeatureCount + kEnergyBucketCount;
}

// One-hot distance/type + numeric growth_stage per ray, then energy one-hot.
// Only the first feature_size(vision.ray_count) floats are meaningful.
std::array<float, kMaxFeatureSize> encode_features(const Vision& vision,
                                                   int energy, int energy_max);

}  // namespace agent
}  // namespace life_worlds
