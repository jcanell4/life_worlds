#pragma once

#include "life_worlds/core/visual.hpp"

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

// Energy an entity holds and the most it can hold. Modules that count in
// whole units (ql_agent) store integral values.
struct Energy {
  double value = 0.0;
  double max = 0.0;
};

// Resolved look of an entity, set by its module when it spawns. Render layers
// draw from this component alone, so they do not depend on the module: an
// entity is drawn by the layer named here, and an entity without Visual is not
// drawn at all.
struct Visual {
  Shape shape = Shape::Circle;
  Rgba color;
  RenderLayerId layer = 0;
};

// Fill in the parts a "visual" config block left unset with the module's
// defaults for this entity.
inline Visual resolve_visual(const VisualConfig& config, Shape default_shape,
                             Rgba default_color) {
  return Visual{config.shape.value_or(default_shape),
                config.color.value_or(default_color), config.layer};
}

}  // namespace life_worlds
