#pragma once

#include "life_worlds/entity/entity.hpp"
#include "life_worlds/environment/environment.hpp"
#include "life_worlds/simulation/config.hpp"

#include <memory>
#include <string>
#include <vector>

namespace life_worlds {

// Result of load_config: simulation settings, environment, and entities.
struct LoadedConfig {
  SimulationConfig simulation;
  std::unique_ptr<Environment> environment;
  std::vector<std::unique_ptr<Entity>> entities;
};

// Load JSON into simulation settings, environment, and Entity instances.
bool load_config(const std::string& path, LoadedConfig& out,
                 std::string* error_out = nullptr);

}  // namespace life_worlds
