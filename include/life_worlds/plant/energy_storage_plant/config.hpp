#pragma once

#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/common.hpp"

#include <string>
#include <vector>

namespace life_worlds {
namespace energy_storage_plant {

// Species index into EnergyStoragePlantConfig::plant_types (shared plant type).
using plant::PlantKind;

struct PlantTypeConfig {
  std::string name;
  // Number of founder plants placed on free cells at the first step. Each one
  // starts its own lineage.
  int initial_count = 0;
  // Energy each founder starts with (clamped to maximum_energy_limit).
  double initial_energy = 0.0;
  // Energy added to a plant every cycle (until maximum_energy_limit).
  double growing_rate = 1.0;
  // Energy cap of every plant of this species.
  double maximum_energy_limit = 100.0;
  // Max number of cells a single lineage (founder + descendants) may occupy.
  int maximum_cell_occupancy = 1;
  // Fraction of the parent's energy handed to the new plant when it splits;
  // the parent keeps the rest.
  double split_fraction = 0.5;
  // Optional shape / color. Unset color: one color per lineage, shaded by
  // stored energy. Unset shape: hexagon.
  Visual visual;
};

struct EnergyStoragePlantConfig {
  std::vector<PlantTypeConfig> plant_types;
};

}  // namespace energy_storage_plant
}  // namespace life_worlds
