#pragma once

#include <string>
#include <vector>

namespace life_worlds {

// Named integer metric used in tick summaries and the HUD.
struct NamedCount {
  std::string name;
  int value = 0;
};

// Filled by each Entity during a tick; Simulation uses it for logging / HUD.
struct TickAccumulator {
  int births = 0;
  int deaths = 0;
  std::vector<NamedCount> alive_by_name;
  std::vector<NamedCount> births_by_name;
  std::vector<NamedCount> deaths_by_name;
  std::vector<NamedCount> energy_by_agent;
  std::vector<NamedCount> eaten_by_name;
  std::vector<NamedCount> cut_by_name;

  // Reset metric lists for the next simulation cycle (events cleared separately).
  void clear() {
    births = 0;
    deaths = 0;
    alive_by_name.clear();
    births_by_name.clear();
    deaths_by_name.clear();
    energy_by_agent.clear();
    eaten_by_name.clear();
    cut_by_name.clear();
  }
};

}  // namespace life_worlds
