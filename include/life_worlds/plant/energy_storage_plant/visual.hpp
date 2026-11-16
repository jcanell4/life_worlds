#pragma once

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace life_worlds {
namespace energy_storage_plant {

// Base color of a lineage (same lineage → same color).
inline Color lineage_color(std::uint32_t lineage_id) {
  static const std::array<Color, 8> palette{{
      Color{0, 150, 160, 255},
      Color{150, 70, 170, 255},
      Color{210, 130, 20, 255},
      Color{40, 110, 200, 255},
      Color{190, 60, 90, 255},
      Color{70, 150, 60, 255},
      Color{160, 140, 20, 255},
      Color{90, 90, 180, 255},
  }};
  return palette[static_cast<std::size_t>(lineage_id) % palette.size()];
}

// Scale a color from dim (empty) to full brightness (energy at its cap).
inline Color shade_by_energy(Color base, double fraction) {
  const double f = 0.35 + 0.65 * std::clamp(fraction, 0.0, 1.0);
  return Color{static_cast<unsigned char>(base.r * f),
               static_cast<unsigned char>(base.g * f),
               static_cast<unsigned char>(base.b * f), base.a};
}

}  // namespace energy_storage_plant
}  // namespace life_worlds
