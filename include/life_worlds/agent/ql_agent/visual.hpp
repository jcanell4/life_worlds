#pragma once

#include "life_worlds/core/visual.hpp"

namespace life_worlds {
namespace ql_agent {

inline Rgba agent_color() { return Rgba{200, 90, 40, 255}; }

inline Shape default_shape() { return Shape::Triangle; }

}  // namespace ql_agent
}  // namespace life_worlds
