#pragma once

#include "life_worlds/core/visual.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace life_worlds {
namespace energy_storage_plant {

// Default color of a plant: one per lineage (same lineage → same color).
inline Rgba lineage_color(std::uint32_t lineage_id) {
  static const std::array<Rgba, 8> palette{{
      Rgba{0, 150, 160, 255},
      Rgba{150, 70, 170, 255},
      Rgba{210, 130, 20, 255},
      Rgba{40, 110, 200, 255},
      Rgba{190, 60, 90, 255},
      Rgba{70, 150, 60, 255},
      Rgba{160, 140, 20, 255},
      Rgba{90, 90, 180, 255},
  }};
  return palette[static_cast<std::size_t>(lineage_id) % palette.size()];
}

// Default shape of a plant.
inline Shape default_shape() { return Shape::Hexagon; }

}  // namespace energy_storage_plant
}  // namespace life_worlds
