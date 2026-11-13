#include "life_worlds/environment/grid/adapters.hpp"

#include "life_worlds/agent/ql_agent/components.hpp"
#include "life_worlds/agent/ql_agent/config.hpp"
#include "life_worlds/agent/ql_agent/q_table.hpp"
#include "life_worlds/plant/random_plant/components.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace life_worlds {
namespace grid {
namespace {

constexpr std::array<std::pair<int, int>, ql_agent::kDirectionCount> kDelta{{
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
}};

ql_agent::ObjectType plant_object_type(const std::string& name) {
  if (name == "herb") {
    return ql_agent::ObjectType::Herb;
  }
  if (name == "shrub") {
    return ql_agent::ObjectType::Shrub;
  }
  return ql_agent::ObjectType::EmptyEdge;
}

// Cast four orthogonal rays using grid occupancy queries.
ql_agent::Vision observe_grid(const GridEnvironment& env,
                              const entt::registry& registry, int x, int y,
                              int max_vision, entt::entity self) {
  ql_agent::Vision vision{};
  const int vision_cap = std::max(1, max_vision);
  for (int d = 0; d < ql_agent::kDirectionCount; ++d) {
    const auto [dx, dy] = kDelta[static_cast<std::size_t>(d)];
    ql_agent::VisionRay ray{ql_agent::distance_bucket(vision_cap),
                            static_cast<int>(ql_agent::ObjectType::EmptyEdge)};
    for (int step = 1; step <= vision_cap; ++step) {
      const int nx = x + dx * step;
      const int ny = y + dy * step;
      if (!env.in_bounds(nx, ny)) {
        ray.distance = ql_agent::distance_bucket(step);
        ray.object_type = static_cast<int>(ql_agent::ObjectType::EmptyEdge);
        break;
      }
      if (env.cell_has_agent(registry, nx, ny, self)) {
        ray.distance = ql_agent::distance_bucket(step);
        ray.object_type = static_cast<int>(ql_agent::ObjectType::QlAgent);
        break;
      }
      const entt::entity plant = env.plant_at(registry, nx, ny);
      if (plant != entt::null) {
        ray.distance = ql_agent::distance_bucket(step);
        if (registry.all_of<random_plant::SpeciesName>(plant)) {
          ray.object_type = static_cast<int>(plant_object_type(
              registry.get<random_plant::SpeciesName>(plant).name));
        } else {
          ray.object_type = static_cast<int>(ql_agent::ObjectType::EmptyEdge);
        }
        break;
      }
      if (step == vision_cap) {
        ray.distance = ql_agent::distance_bucket(vision_cap);
        ray.object_type = static_cast<int>(ql_agent::ObjectType::EmptyEdge);
      }
    }
    vision[static_cast<std::size_t>(d)] = ray;
  }
  return vision;
}

}  // namespace

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
                        entt::entity self) {
    return observe_grid(env, registry, x, y, range, self);
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
