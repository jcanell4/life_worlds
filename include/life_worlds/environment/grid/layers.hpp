#pragma once

#include "life_worlds/environment/environment.hpp"
#include "life_worlds/environment/grid/environment.hpp"

namespace life_worlds {
namespace grid {

// Register the standard visual layers for a grid life-worlds app.
void register_default_render_layers(GridEnvironment& env);

}  // namespace grid
}  // namespace life_worlds
