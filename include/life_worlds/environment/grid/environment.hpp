#pragma once

#include "life_worlds/environment/environment.hpp"
#include "life_worlds/environment/grid/action.hpp"
#include "life_worlds/environment/grid/config.hpp"
#include "life_worlds/environment/grid/vision.hpp"

#include <entt/entt.hpp>

#include <memory>
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

  // Install orthogonal (4) and octant (8) sensors sharing the same plant lookup.
  void set_vision_sensors(std::unique_ptr<VisionSensor> orthogonal,
                          std::unique_ptr<VisionSensor> octant);

  // Cast rays with the sensor matching `ray_count` (4 or 8).
  agent::Vision observe(const entt::registry& registry, int x, int y,
                        int max_vision, entt::entity self, int ray_count) const;

 private:
  GridConfig config_;
  std::unique_ptr<VisionSensor> orthogonal_vision_;
  std::unique_ptr<VisionSensor> octant_vision_;
};

}  // namespace grid
}  // namespace life_worlds
