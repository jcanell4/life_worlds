#include "life_worlds/agent/features.hpp"

#include <algorithm>
#include <cstddef>

namespace life_worlds {
namespace agent {
namespace {

void write_one_hot(float* out, int index, int count) {
  for (int i = 0; i < count; ++i) {
    out[i] = (i == index) ? 1.0f : 0.0f;
  }
}

}  // namespace

std::array<float, kMaxFeatureSize> encode_features(const Vision& vision,
                                                   int energy, int energy_max) {
  std::array<float, kMaxFeatureSize> features{};
  float* cursor = features.data();
  const int rays = std::clamp(vision.ray_count, 1, kMaxVisionRays);
  for (int d = 0; d < rays; ++d) {
    const auto& ray = vision[static_cast<std::size_t>(d)];
    const int dist = std::clamp(ray.distance, 0, kDistanceBucketCount - 1);
    const int type = std::clamp(ray.object_type, 0, kObjectTypeCount - 1);
    write_one_hot(cursor, dist, kDistanceBucketCount);
    cursor += kDistanceBucketCount;
    write_one_hot(cursor, type, kObjectTypeCount);
    cursor += kObjectTypeCount;
    *cursor++ = ray.growth_stage;
  }
  write_one_hot(cursor, energy_bucket(energy, energy_max), kEnergyBucketCount);
  return features;
}

}  // namespace agent
}  // namespace life_worlds
