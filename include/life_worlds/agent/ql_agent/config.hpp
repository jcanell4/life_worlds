#pragma once

#include "life_worlds/core/components.hpp"
#include "life_worlds/environment/grid/action.hpp"

#include <array>
#include <string>

namespace life_worlds {
namespace ql_agent {

enum class Mode {
  Train,
  Eval,
};

inline constexpr int kActionCount = grid::kActionCount;

using Action = grid::Action;

struct QlAgentConfig {
  Mode mode = Mode::Train;
  int agent_count = 1;
  int energy_max = 100;
  int energy_drain = 5;
  int herb_energy_delta = 10;
  int shrub_energy_delta = -10;
  std::array<double, 2> ingestion_factor{{1.0, 1.0}};
  double alpha = 0.1;
  double gamma = 0.9;
  double epsilon = 0.1;
  // Reward when a move leaves the grid (precipice).
  double precipice_reward = -20.0;
  // Orthogonal (4) or octant (8) vision rays; shared by all agents of this process.
  int vision_rays = 4;
  // 0 = use environment grid_size at runtime.
  int max_vision = 0;
  std::string q_table_path = "config/ql_qtable.bin";
  bool reuse_q_table = false;
  VisualConfig visual;
};

}  // namespace ql_agent
}  // namespace life_worlds
