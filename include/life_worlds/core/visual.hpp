#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

namespace life_worlds {

// Value types used by the Visual component (core/components.hpp).

// Shapes a plant or agent can be drawn with.
enum class Shape : std::uint8_t {
  Circle,
  Square,
  Triangle,
  Diamond,
  Hexagon,
};

// Raylib-free RGBA color (0-255 per channel).
struct Rgba {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

// Identifies one of the render layers of the active environment. Modules copy
// it from their config onto their entities without knowing which layer it is;
// only the environment and the composition root give it a meaning.
using RenderLayerId = std::uint8_t;

// Look requested for a plant species or agent group ("visual" in the JSON).
// A missing shape / color means "use the module's default" for that part. The
// layer is always set: the composition root fills in the module's default.
struct VisualConfig {
  std::optional<Shape> shape;
  std::optional<Rgba> color;
  RenderLayerId layer = 0;
};

// Scale a color from dim (fraction 0) to full brightness (fraction 1).
inline Rgba shade_by_fraction(Rgba base, double fraction) {
  const double f = 0.35 + 0.65 * std::clamp(fraction, 0.0, 1.0);
  return Rgba{static_cast<std::uint8_t>(base.r * f),
              static_cast<std::uint8_t>(base.g * f),
              static_cast<std::uint8_t>(base.b * f), base.a};
}

// Text color that stays readable over the given fill: dark over light fills,
// light over dark ones.
inline Rgba contrast_text_color(Rgba fill) {
  const double luminance = 0.299 * fill.r + 0.587 * fill.g + 0.114 * fill.b;
  return luminance > 140.0 ? Rgba{20, 20, 20, 255} : Rgba{240, 240, 240, 255};
}

}  // namespace life_worlds
