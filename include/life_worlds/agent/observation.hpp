#pragma once

#include "life_worlds/environment/grid/action.hpp"

#include <array>
#include <cstddef>

namespace life_worlds {
namespace agent {

// Movement directions stay at 4; vision may use up to 8 rays.
inline constexpr int kMaxVisionRays = 8;
inline constexpr int kObjectTypeCount = 4;  // herb, shrub, agent, empty_edge
inline constexpr int kDistanceBucketCount = 4;
inline constexpr int kEnergyBucketCount = 4;

// First object hit by a vision ray.
enum class ObjectType : int {
  Herb = 0,
  Shrub = 1,
  Agent = 2,
  EmptyEdge = 3,
};

// One directional observation: discretized distance + first object hit +
// optional plant growth stage in [0, 1] (0 = none / edge / agent).
struct VisionRay {
  int distance = 0;
  int object_type = static_cast<int>(ObjectType::EmptyEdge);
  float growth_stage = 0.f;
};

// Up to kMaxVisionRays rays; ray_count is 4 (orthogonal) or 8 (octant).
struct Vision {
  std::array<VisionRay, kMaxVisionRays> rays{};
  int ray_count = 4;

  VisionRay& operator[](std::size_t i) { return rays[i]; }
  const VisionRay& operator[](std::size_t i) const { return rays[i]; }
};

// Map raw cell distance to a bucket: 1 -> 0, 2..4 -> 1, 5..10 -> 2, >10 -> 3.
int distance_bucket(int cells);

// Map energy level to a bucket:
// [75%,100%] -> 3, [50%,75%) -> 2, [25%,50%) -> 1, [0%,25%) -> 0.
int energy_bucket(int energy, int energy_max);

}  // namespace agent
}  // namespace life_worlds
