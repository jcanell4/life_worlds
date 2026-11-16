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
inline constexpr int kDirectionCount = grid::kDirectionCount;
inline constexpr int kObjectTypeCount = 4;  // herb, shrub, ql_agent, empty_edge
inline constexpr int kDistanceBucketCount = 4;
inline constexpr int kEnergyBucketCount = 4;

using Action = grid::Action;

enum class ObjectType : int {
  Herb = 0,
  Shrub = 1,
  QlAgent = 2,
  EmptyEdge = 3,
};

struct QlAgentConfig {
  Mode mode = Mode::Train;
  int agent_count = 1;
  int energy_max = 100;
  int energy_drain = 5;
  int herb_energy_delta = 10;
  int shrub_energy_delta = -10;
  // Multiplier on energy delta when eating: [herb, shrub]. Default 1.0.
  std::array<double, 2> ingestion_factor{{1.0, 1.0}};
  double alpha = 0.1;
  double gamma = 0.9;
  double epsilon = 0.1;
  // 0 = use environment grid_size at runtime.
  int max_vision = 0;
  std::string q_table_path = "config/ql_qtable.bin";
  // If true (or mode is eval), load q_table_path at startup when the file exists.
  bool reuse_q_table = false;
  // Optional shape / color for every agent; unset parts use the orange triangle.
  Visual visual;
};

}  // namespace ql_agent
}  // namespace life_worlds
