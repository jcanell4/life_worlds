#pragma once

#include "life_worlds/core/visual.hpp"

namespace life_worlds {
namespace c51_agent {

// Default color of a C51 agent (blue, distinct from the orange QL agents).
inline Rgba agent_color() { return Rgba{40, 90, 200, 255}; }

// Default shape of a C51 agent.
inline Shape default_shape() { return Shape::Triangle; }

}  // namespace c51_agent
}  // namespace life_worlds
