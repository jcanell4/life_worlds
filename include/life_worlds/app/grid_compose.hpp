#pragma once

#include "life_worlds/config_loader.hpp"

#include <nlohmann/json.hpp>

namespace life_worlds {
namespace app {

// Wire a grid environment: ports for entities + default render layers.
void compose_grid_app(const nlohmann::json& root, LoadedConfig& loaded);

}  // namespace app
}  // namespace life_worlds
