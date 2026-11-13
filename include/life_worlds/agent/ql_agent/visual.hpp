#pragma once

#include <raylib.h>

namespace life_worlds {
namespace ql_agent {

// Triangle fill for active QL agents.
inline Color agent_color() { return Color{200, 90, 40, 255}; }

// Energy label color drawn under the agent.
inline Color energy_text_color() { return Color{40, 30, 20, 255}; }

}  // namespace ql_agent
}  // namespace life_worlds
