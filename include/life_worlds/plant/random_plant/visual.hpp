#pragma once

#include "life_worlds/core/visual.hpp"
#include "life_worlds/plant/random_plant/components.hpp"

#include <array>
#include <cstddef>

namespace life_worlds {
namespace random_plant {

// Default color of a living plant: palette entry by species index.
inline Rgba color_for_kind(PlantKind kind) {
  static const std::array<Rgba, 8> palette{{
      Rgba{72, 168, 74, 255},
      Rgba{46, 92, 58, 255},
      Rgba{120, 180, 90, 255},
      Rgba{34, 120, 80, 255},
      Rgba{90, 140, 60, 255},
      Rgba{60, 110, 70, 255},
      Rgba{150, 190, 100, 255},
      Rgba{40, 80, 50, 255},
  }};
  return palette[static_cast<std::size_t>(kind) % palette.size()];
}

// Default shape of a living plant: even species index → circle, odd → square.
inline Shape default_shape(PlantKind kind) {
  return (kind % 2) == 0 ? Shape::Circle : Shape::Square;
}

// Residue colors: gray = eaten, brown = cut.
inline Rgba residue_color(ResidueKind kind) {
  return kind == ResidueKind::Cut ? Rgba{139, 90, 43, 255}
                                  : Rgba{150, 150, 150, 255};
}

// Colorblind-friendly label: E = eaten, K = cut (killed/cut).
inline const char* residue_label(ResidueKind kind) {
  return kind == ResidueKind::Cut ? "K" : "E";
}

// High-contrast label color over residue shapes.
inline Rgba residue_label_color() { return Rgba{20, 20, 20, 255}; }

}  // namespace random_plant
}  // namespace life_worlds
