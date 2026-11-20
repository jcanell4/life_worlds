#pragma once

#include "life_worlds/core/components.hpp"
#include "life_worlds/core/visual.hpp"
#include "life_worlds/environment/environment.hpp"
#include "life_worlds/environment/grid/environment.hpp"

#include <entt/entt.hpp>

#include <cmath>
#include <string>

namespace life_worlds {
namespace grid {

// Render layers of the grid environment: the ways this environment can draw
// the world, in cells. Another kind of environment defines its own set.
//
// Draw order is fixed, bottom to top: background, residue, then the
// selectable layers in the order listed below.

// Selectable layers. An entity is drawn by the one stored in Visual::layer,
// which comes from "visual.render_layer" in the config (or from the default
// the composition root gives to its module). They read only core components,
// so any module can use any of them.

// Shape and color.
inline constexpr RenderLayerId kPlainLayer = 0;
// Shape with brightness following Energy value / max, and the value centered.
inline constexpr RenderLayerId kLevelLayer = 1;
// Shape and color, with the Energy value under it.
inline constexpr RenderLayerId kValueBelowLayer = 2;

// Id of the selectable layer called `name` ("plain", "level", "value_below").
// Throws std::runtime_error if this environment has no such layer.
RenderLayerId render_layer_id(const std::string& name);

// Register every grid layer on `env`, in draw order.
void register_render_layers(GridEnvironment& env);

// One factory per layer (each in its own file).
RenderLayer make_background_layer(const GridEnvironment& env);  // empty cells
RenderLayer make_residue_layer(const GridEnvironment& env);     // eaten / cut
RenderLayer make_plain_layer(const GridEnvironment& env);
RenderLayer make_level_layer(const GridEnvironment& env);
RenderLayer make_value_below_layer(const GridEnvironment& env);

// Call draw(entity, position, visual) for every entity assigned to `layer`.
template <typename Draw>
void for_each_in_layer(const entt::registry& registry, RenderLayerId layer,
                       Draw&& draw) {
  auto view = registry.view<const Position, const Visual>();
  for (auto entity : view) {
    const auto& visual = view.template get<const Visual>(entity);
    if (visual.layer == layer) {
      draw(entity, view.template get<const Position>(entity), visual);
    }
  }
}

// Energy value as shown in a label: rounded to whole units.
inline std::string energy_label(const Energy& energy) {
  return std::to_string(std::lround(energy.value));
}

}  // namespace grid
}  // namespace life_worlds
