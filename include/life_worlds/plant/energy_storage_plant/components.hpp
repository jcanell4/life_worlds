#pragma once

#include "life_worlds/plant/energy_storage_plant/config.hpp"

#include <cstdint>

namespace life_worlds {
namespace energy_storage_plant {

// Marker: entity is a living plant managed by EnergyStoragePlantProcess.
// The species is stored in the shared plant::PlantTypeIndex component.
struct EnergyPlant {};

// Energy currently stored and the cap copied from the species config.
struct StoredEnergy {
  double value = 0.0;
  double max = 0.0;
};

// Origin of the plant: all descendants of a founder share its lineage id.
struct Lineage {
  std::uint32_t id = 0;
};

}  // namespace energy_storage_plant
}  // namespace life_worlds
