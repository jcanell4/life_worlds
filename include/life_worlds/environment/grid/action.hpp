#pragma once

#include <string>

namespace life_worlds {
namespace grid {

inline constexpr int kActionCount = 6;
inline constexpr int kDirectionCount = 4;

// Discrete actions understood by GridEnvironment.
enum class Action : int {
  Up = 0,
  Down = 1,
  Left = 2,
  Right = 3,
  Eat = 4,
  Cut = 5,
};

// Parameters the acting entity supplies so the environment can resolve Eat.
struct EatParams {
  int herb_energy_delta = 10;
  int shrub_energy_delta = -10;
  double herb_ingestion = 1.0;
  double shrub_ingestion = 1.0;
  int energy_max = 100;
};

// Result of GridEnvironment::apply_action for the acting entity to learn from.
struct ActionOutcome {
  double food_reward = 0.0;
  int energy_after = 0;
  bool moved = false;
  bool eaten = false;
  bool cut = false;
  // True when a move tried to leave the grid (precipice).
  bool fell = false;
  std::string species_name;
  std::string detail;
};

}  // namespace grid
}  // namespace life_worlds
