#pragma once

#include <cstdint>

namespace life_worlds {
namespace plant {

// Components shared by every plant module.

// Species index into a plant module's `plant_types` config list.
using PlantKind = std::uint8_t;

// Which species a plant is (index into its module's plant_types). Every plant
// module attaches this same component; a module tells its own plants apart
// with its marker component (random_plant::Plant, energy_storage_plant::
// EnergyPlant), so views must always include the marker.
struct PlantTypeIndex {
  PlantKind kind = 0;
};

}  // namespace plant
}  // namespace life_worlds
