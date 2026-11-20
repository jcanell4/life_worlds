#pragma once

#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/common.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace life_worlds {
namespace random_plant {

// Capacity of the per-stage tables; PlantTypeConfig::stage_count must not exceed it.
inline constexpr std::size_t kMaxStages = 8;

// Species index into RandomPlantConfig::plant_types (shared plant type).
using plant::PlantKind;

struct PlantTypeConfig {
  std::string name;
  // Minimum number of living individuals of this species. Every tick, if the
  // population is below it, new plants are seeded in free cells to reach it.
  // 0 = no seeding: the species can go extinct.
  int min_population = 0;
  // Soft population cap. Reproduction and seeding will not push the living
  // count above this value. 0 = no maximum.
  int max_population = 0;
  // Number of age stages for this species. It is the number of valid entries
  // in the per-stage tables below; plants older than the last stage keep using
  // the last entry.
  int stage_count = 0;
  // Death probability per cycle for stage 0, 1, ..., stage_count - 1.
  std::array<double, kMaxStages> death_rate_by_stage{};
  // Default offspring probability per cycle for each stage. Only used as the
  // starting value for seeded plants; each individual then carries and passes
  // on its own copy (see Fertility).
  std::array<double, kMaxStages> offspring_rate_by_stage{};
  // Duration of each stage in time units. Cycles for stage i =
  // max(1, time_units_by_stage[i]) * cycles_per_time_unit.
  // Values <= 0 are treated as 1.
  std::array<int, kMaxStages> time_units_by_stage{};
  // Optional shape / color / layer; unset parts fall back to module defaults.
  VisualConfig visual;
};

struct RandomPlantConfig {
  // Max absolute random change (uniform, +/-) applied to each per-stage
  // offspring probability when a child inherits it from its parent. 0 = exact copy.
  double offspring_inheritance_jitter = 0.01;
  std::vector<PlantTypeConfig> plant_types;
};

}  // namespace random_plant
}  // namespace life_worlds
