#include "life_worlds/environment/grid/adapters.hpp"

#include <algorithm>

namespace life_worlds {
namespace grid {

OccupancyPort make_occupancy_port(GridEnvironment& env) {
  OccupancyPort port;
  port.free_cells = [&env](const entt::registry& registry) {
    return env.free_cells(registry);
  };
  return port;
}

AgentWorldPort make_agent_world_port(GridEnvironment& env) {
  AgentWorldPort port;
  port.default_vision_range = [&env]() { return std::max(1, env.grid_size()); };
  port.observe = [&env](const entt::registry& registry, int x, int y, int range,
                        entt::entity self, int ray_count) {
    return env.observe(registry, x, y, range, self, ray_count);
  };
  port.apply_action = [&env](entt::registry& registry, entt::entity agent,
                             Action action, const EatParams& eat,
                             events::EventQueue& events) {
    return env.apply_action(registry, agent, action, eat, events);
  };
  port.reset_position = [&env](entt::registry& registry, entt::entity agent,
                               std::mt19937& rng) {
    env.reset_agent_position(registry, agent, rng);
  };
  port.free_spawn_cells = [&env](const entt::registry& registry) {
    return env.free_cells(registry);
  };
  return port;
}

}  // namespace grid
}  // namespace life_worlds
