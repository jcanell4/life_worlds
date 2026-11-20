#pragma once

#include "life_worlds/core/visual.hpp"
#include "life_worlds/environment/grid/action.hpp"

#include <array>
#include <string>
#include <vector>

namespace life_worlds {
namespace dqn_agent {

enum class Mode {
  Train,
  Eval,
};

inline constexpr int kActionCount = grid::kActionCount;

using Action = grid::Action;

struct DqnAgentConfig {
  Mode mode = Mode::Train;
  int agent_count = 1;
  int energy_max = 100;
  int energy_drain = 5;
  int herb_energy_delta = 10;
  int shrub_energy_delta = -10;
  std::array<double, 2> ingestion_factor{{1.0, 1.0}};
  double gamma = 0.9;
  double epsilon = 0.1;
  double precipice_reward = -20.0;
  // Orthogonal (4) or octant (8) vision rays; sizes the network input.
  int vision_rays = 4;
  // 0 = use environment grid_size at runtime.
  int max_vision = 0;

  std::vector<int> hidden_sizes{{128, 128}};
  double learning_rate = 1e-3;
  int batch_size = 64;
  int replay_capacity = 50000;
  int learn_start = 1000;
  int train_every = 4;
  int target_update_every = 500;

  std::string model_path = "config/dqn_random_plant_1.pt";
  // If true (or mode is eval), load model_path at startup when the file exists.
  bool reuse_model = false;

  VisualConfig visual;
};

}  // namespace dqn_agent
}  // namespace life_worlds
