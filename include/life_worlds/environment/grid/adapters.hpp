#pragma once

#include "life_worlds/environment/grid/environment.hpp"
#include "life_worlds/ports/agent_world_port.hpp"
#include "life_worlds/ports/occupancy_port.hpp"

namespace life_worlds {
namespace grid {

// Build an occupancy port that delegates to GridEnvironment::free_cells.
OccupancyPort make_occupancy_port(GridEnvironment& env);

// Build an agent world port (observe / apply / spawn / reset) for this grid.
AgentWorldPort make_agent_world_port(GridEnvironment& env);

}  // namespace grid
}  // namespace life_worlds
