#pragma once

#include "life_worlds/plant/random_plant/components.hpp"

#include <raylib.h>

#include <array>
#include <cstdint>

namespace life_worlds {
namespace random_plant {

// Palette color for a living plant species index.
inline Color color_for_kind(std::uint8_t kind) {
  static const std::array<Color, 8> palette{{
      Color{72, 168, 74, 255},
      Color{46, 92, 58, 255},
      Color{120, 180, 90, 255},
      Color{34, 120, 80, 255},
      Color{90, 140, 60, 255},
      Color{60, 110, 70, 255},
      Color{150, 190, 100, 255},
      Color{40, 80, 50, 255},
  }};
  return palette[static_cast<std::size_t>(kind) % palette.size()];
}

// Residue colors: gray = eaten, brown = cut.
inline Color residue_color(ResidueKind kind) {
  return kind == ResidueKind::Cut ? Color{139, 90, 43, 255}
                                  : Color{150, 150, 150, 255};
}

// Colorblind-friendly label: E = eaten, K = cut (killed/cut).
inline const char* residue_label(ResidueKind kind) {
  return kind == ResidueKind::Cut ? "K" : "E";
}

// High-contrast label color over residue shapes.
inline Color residue_label_color() { return Color{20, 20, 20, 255}; }

// Even kind → circle, odd kind → square (used by grid renderer).
inline bool uses_circle_shape(int kind) { return (kind % 2) == 0; }

}  // namespace random_plant
}  // namespace life_worlds
