#pragma once

#include "life_worlds/agent/ql_agent/q_table.hpp"
#include "life_worlds/environment/grid/action.hpp"
#include "life_worlds/simulation/events/event_queue.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <random>
#include <utility>
#include <vector>

namespace life_worlds {

// Sensing / actuation surface a QL agent needs from the composed world.
struct AgentWorldPort {
  std::function<int()> default_vision_range;

  std::function<ql_agent::Vision(const entt::registry&, int x, int y, int range,
                                 entt::entity self)>
      observe;

  std::function<grid::ActionOutcome(
      entt::registry&, entt::entity, grid::Action, const grid::EatParams&,
      events::EventQueue&)>
      apply_action;

  std::function<void(entt::registry&, entt::entity, std::mt19937&)>
      reset_position;

  std::function<std::vector<std::pair<int, int>>(const entt::registry&)>
      free_spawn_cells;
};

}  // namespace life_worlds
