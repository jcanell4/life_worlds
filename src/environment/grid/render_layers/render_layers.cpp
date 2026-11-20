#include "life_worlds/environment/grid/render_layers/render_layers.hpp"

#include <stdexcept>

namespace life_worlds {
namespace grid {

RenderLayerId render_layer_id(const std::string& name) {
  if (name == "plain") {
    return kPlainLayer;
  }
  if (name == "level") {
    return kLevelLayer;
  }
  if (name == "value_below") {
    return kValueBelowLayer;
  }
  throw std::runtime_error(
      "grid render layer must be plain, level or value_below (got \"" + name +
      "\")");
}

void register_render_layers(GridEnvironment& env) {
  env.add_render_layer(make_background_layer(env));
  env.add_render_layer(make_residue_layer(env));
  env.add_render_layer(make_plain_layer(env));
  env.add_render_layer(make_level_layer(env));
  env.add_render_layer(make_value_below_layer(env));
}

}  // namespace grid
}  // namespace life_worlds
