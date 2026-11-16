#pragma once

#include "life_worlds/core/visual.hpp"

#include <optional>

namespace life_worlds {

// Components used by more than one kind of module.

// Cell coordinates on the N×N world grid.
struct Position {
  int x = 0;
  int y = 0;
};

// Age measured in simulation cycles.
struct Age {
  int cycles = 0;
};

// Optional look of a plant species or agent group ("visual" in the JSON),
// copied onto each entity when it spawns. A missing field means "use the
// module's default" for that part.
struct Visual {
  std::optional<Shape> shape;
  std::optional<Rgba> color;
};

}  // namespace life_worlds
