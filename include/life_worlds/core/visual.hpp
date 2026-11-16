#pragma once

#include <cstdint>

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

}  // namespace life_worlds
