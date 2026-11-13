#pragma once

#include "life_worlds/environment/environment.hpp"
#include "life_worlds/environment/grid/action.hpp"
#include "life_worlds/environment/grid/config.hpp"

#include <entt/entt.hpp>

#include <random>
#include <utility>
#include <vector>

namespace life_worlds {
namespace grid {

// Discrete N×N grid world: occupancy queries, action resolution, rendering.
class GridEnvironment final : public Environment {
 public:
  explicit GridEnvironment(GridConfig config);

  void step(StepContext& ctx) override;
  DisplayExtent display_extent(const SimulationConfig& display) const override;

  int grid_size() const { return config_.grid_size; }
  const GridConfig& config() const { return config_; }

  // True if (x, y) is inside the grid.
  bool in_bounds(int x, int y) const;

  // Living plant at cell, or entt::null.
  entt::entity plant_at(const entt::registry& registry, int x, int y) const;

  // Active QL agent (other than self) at cell.
  bool cell_has_agent(const entt::registry& registry, int x, int y,
                      entt::entity self) const;

  // Blocked for movement by plants or other agents.
  bool cell_blocked(const entt::registry& registry, int x, int y,
                    entt::entity self) const;

  // First orthogonally adjacent living plant.
  entt::entity adjacent_plant(const entt::registry& registry, int x,
                              int y) const;

  // Free cells (no plant, no active agent).
  std::vector<std::pair<int, int>> free_cells(
      const entt::registry& registry) const;

  // Apply a discrete action for agent; mutates world and may emit events.
  ActionOutcome apply_action(entt::registry& registry, entt::entity agent,
                             Action action, const EatParams& eat,
                             events::EventQueue& events) const;

  // Place agent on a random free cell (current cell counted as free for self).
  void reset_agent_position(entt::registry& registry, entt::entity agent,
                            std::mt19937& rng) const;

 private:
  GridConfig config_;
};

}  // namespace grid
}  // namespace life_worlds
