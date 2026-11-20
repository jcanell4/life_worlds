#pragma once

#include "life_worlds/agent/observation.hpp"
#include "life_worlds/plant/random_plant/config.hpp"

#include <entt/entt.hpp>

#include <memory>
#include <vector>

namespace life_worlds {
namespace grid {

class GridEnvironment;

// Casts vision rays into the grid; OrthogonalVision (4) or OctantVision (8).
class VisionSensor {
 public:
  virtual ~VisionSensor() = default;

  virtual int ray_count() const = 0;

  virtual agent::Vision observe(const GridEnvironment& env,
                                const entt::registry& registry, int x, int y,
                                int max_vision, entt::entity self) const = 0;
};

// Shared plant-type table so rays can normalize growth_stage per species.
struct VisionPlantLookup {
  std::vector<random_plant::PlantTypeConfig> plant_types;
  int cycles_per_time_unit = 50;
};

std::unique_ptr<VisionSensor> make_orthogonal_vision(VisionPlantLookup lookup);
std::unique_ptr<VisionSensor> make_octant_vision(VisionPlantLookup lookup);

}  // namespace grid
}  // namespace life_worlds
