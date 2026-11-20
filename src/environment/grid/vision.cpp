#include "life_worlds/environment/grid/vision.hpp"

#include "life_worlds/agent/components.hpp"
#include "life_worlds/core/components.hpp"
#include "life_worlds/environment/grid/environment.hpp"
#include "life_worlds/plant/components.hpp"
#include "life_worlds/plant/random_plant/components.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace life_worlds {
namespace grid {
namespace {

agent::ObjectType plant_object_type(const std::string& name) {
  if (name == "herb") {
    return agent::ObjectType::Herb;
  }
  if (name == "shrub") {
    return agent::ObjectType::Shrub;
  }
  return agent::ObjectType::EmptyEdge;
}

float normalized_growth_stage(const entt::registry& registry,
                              entt::entity plant,
                              const VisionPlantLookup& lookup) {
  if (!registry.all_of<Age, plant::PlantTypeIndex>(plant)) {
    return 0.f;
  }
  const auto kind = registry.get<plant::PlantTypeIndex>(plant).kind;
  if (static_cast<std::size_t>(kind) >= lookup.plant_types.size()) {
    return 0.f;
  }
  const auto& type = lookup.plant_types[static_cast<std::size_t>(kind)];
  const int stage_count = std::max(1, type.stage_count);
  const int stage = random_plant::stage_at(
      registry.get<Age>(plant).cycles, type, lookup.cycles_per_time_unit);
  return static_cast<float>(stage + 1) / static_cast<float>(stage_count);
}

agent::VisionRay cast_ray(const GridEnvironment& env,
                          const entt::registry& registry, int x, int y, int dx,
                          int dy, int vision_cap, entt::entity self,
                          const VisionPlantLookup& lookup) {
  agent::VisionRay ray{agent::distance_bucket(vision_cap),
                       static_cast<int>(agent::ObjectType::EmptyEdge), 0.f};
  for (int step = 1; step <= vision_cap; ++step) {
    const int nx = x + dx * step;
    const int ny = y + dy * step;
    if (!env.in_bounds(nx, ny)) {
      ray.distance = agent::distance_bucket(step);
      ray.object_type = static_cast<int>(agent::ObjectType::EmptyEdge);
      ray.growth_stage = 0.f;
      break;
    }
    if (env.cell_has_agent(registry, nx, ny, self)) {
      ray.distance = agent::distance_bucket(step);
      ray.object_type = static_cast<int>(agent::ObjectType::Agent);
      ray.growth_stage = 0.f;
      break;
    }
    const entt::entity plant = env.plant_at(registry, nx, ny);
    if (plant != entt::null) {
      ray.distance = agent::distance_bucket(step);
      if (registry.all_of<random_plant::SpeciesName>(plant)) {
        ray.object_type = static_cast<int>(plant_object_type(
            registry.get<random_plant::SpeciesName>(plant).name));
        ray.growth_stage = normalized_growth_stage(registry, plant, lookup);
      } else {
        ray.object_type = static_cast<int>(agent::ObjectType::EmptyEdge);
        ray.growth_stage = 0.f;
      }
      break;
    }
    if (step == vision_cap) {
      ray.distance = agent::distance_bucket(vision_cap);
      ray.object_type = static_cast<int>(agent::ObjectType::EmptyEdge);
      ray.growth_stage = 0.f;
    }
  }
  return ray;
}

agent::Vision cast_deltas(
    const GridEnvironment& env, const entt::registry& registry, int x, int y,
    int max_vision, entt::entity self, const VisionPlantLookup& lookup,
    const std::pair<int, int>* deltas, int ray_count) {
  agent::Vision vision{};
  vision.ray_count = ray_count;
  const int vision_cap = std::max(1, max_vision);
  for (int d = 0; d < ray_count; ++d) {
    const auto [dx, dy] = deltas[d];
    vision[static_cast<std::size_t>(d)] = cast_ray(
        env, registry, x, y, dx, dy, vision_cap, self, lookup);
  }
  return vision;
}

class OrthogonalVision final : public VisionSensor {
 public:
  explicit OrthogonalVision(VisionPlantLookup lookup)
      : lookup_(std::move(lookup)) {}

  int ray_count() const override { return 4; }

  agent::Vision observe(const GridEnvironment& env,
                        const entt::registry& registry, int x, int y,
                        int max_vision, entt::entity self) const override {
    // N, S, W, E — same order as historical 4-ray vision.
    static constexpr std::array<std::pair<int, int>, 4> kDelta{{
        {0, -1},
        {0, 1},
        {-1, 0},
        {1, 0},
    }};
    return cast_deltas(env, registry, x, y, max_vision, self, lookup_,
                       kDelta.data(), 4);
  }

 private:
  VisionPlantLookup lookup_;
};

class OctantVision final : public VisionSensor {
 public:
  explicit OctantVision(VisionPlantLookup lookup)
      : lookup_(std::move(lookup)) {}

  int ray_count() const override { return 8; }

  agent::Vision observe(const GridEnvironment& env,
                        const entt::registry& registry, int x, int y,
                        int max_vision, entt::entity self) const override {
    // N, NE, E, SE, S, SW, W, NW.
    static constexpr std::array<std::pair<int, int>, 8> kDelta{{
        {0, -1},
        {1, -1},
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1},
    }};
    return cast_deltas(env, registry, x, y, max_vision, self, lookup_,
                       kDelta.data(), 8);
  }

 private:
  VisionPlantLookup lookup_;
};

}  // namespace

std::unique_ptr<VisionSensor> make_orthogonal_vision(VisionPlantLookup lookup) {
  return std::make_unique<OrthogonalVision>(std::move(lookup));
}

std::unique_ptr<VisionSensor> make_octant_vision(VisionPlantLookup lookup) {
  return std::make_unique<OctantVision>(std::move(lookup));
}

}  // namespace grid
}  // namespace life_worlds
