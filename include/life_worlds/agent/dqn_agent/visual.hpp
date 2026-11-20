#pragma once

#include "life_worlds/core/visual.hpp"

namespace life_worlds {
namespace dqn_agent {

// Default color of a DQN agent (green, distinct from orange QL / blue C51).
inline Rgba agent_color() { return Rgba{40, 160, 80, 255}; }

// Default shape of a DQN agent.
inline Shape default_shape() { return Shape::Triangle; }

}  // namespace dqn_agent
}  // namespace life_worlds
