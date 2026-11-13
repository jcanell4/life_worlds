#pragma once

#include "life_worlds/plant/random_plant/config.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace life_worlds {
namespace random_plant {

// Identifies which species this plant entity is (index into plant_types).
struct PlantType {
  PlantKind kind = 0;
};

// Species name copied at spawn (e.g. "herb", "shrub") for other modules.
struct SpeciesName {
  std::string name;
};

// Remains after a living plant is eaten or cut, until a new living plant spawns.
enum class ResidueKind : std::uint8_t {
  Eaten = 0,  // drawn gray
  Cut = 1,    // drawn brown
};

struct PlantResidue {
  ResidueKind kind = ResidueKind::Eaten;
  PlantKind species = 0;
};

// Return the age stage index for a plant with the given cycle count.
// Each stage lasts max(1, time_units_by_stage[i]) * max(1, cycles_per_time_unit)
// cycles, except the last stage which has no upper bound.
inline int stage_at(int cycles, const PlantTypeConfig& type,
                    int cycles_per_time_unit) {
  const int stage_count =
      std::clamp(type.stage_count, 0, static_cast<int>(kMaxStages));
  if (stage_count <= 0) {
    return 0;
  }
  const int unit_cycles = std::max(1, cycles_per_time_unit);
  int remaining = std::max(0, cycles);
  for (int i = 0; i < stage_count - 1; ++i) {
    const int units =
        std::max(1, type.time_units_by_stage[static_cast<std::size_t>(i)]);
    const int duration = units * unit_cycles;
    if (remaining < duration) {
      return i;
    }
    remaining -= duration;
  }
  return stage_count - 1;
}

// Per-individual offspring probability (per cycle) for each age stage.
struct Fertility {
  std::array<double, kMaxStages> rate_by_stage{};

  // Offspring probability for the given stage, clamped to [0, 1].
  double rate_for_stage(int stage, int stage_count) const {
    if (stage_count <= 0) {
      return 0.0;
    }
    const int index = std::clamp(stage, 0, stage_count - 1);
    return std::clamp(rate_by_stage[static_cast<std::size_t>(index)], 0.0, 1.0);
  }
};

// Marker: entity is a living plant managed by RandomPlantProcess.
struct Plant {};

}  // namespace random_plant
}  // namespace life_worlds
