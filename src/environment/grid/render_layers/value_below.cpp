#include "life_worlds/environment/grid/render_layers/cell_canvas.hpp"
#include "life_worlds/environment/grid/render_layers/render_layers.hpp"

namespace life_worlds {
namespace grid {
namespace {

// Dark text: the label sits on the empty-cell fill, not on the shape.
Rgba label_color() { return Rgba{40, 30, 20, 255}; }

}  // namespace

// "value_below": the shape in its color with the Energy value under it. An
// entity without Energy is drawn as in "plain".
RenderLayer make_value_below_layer(const GridEnvironment& env) {
  return [&env](const entt::registry& registry,
                const RenderContext& render_ctx) {
    const CellCanvas canvas(env, render_ctx);
    for_each_in_layer(
        registry, kValueBelowLayer,
        [&](entt::entity entity, const Position& pos, const Visual& visual) {
          canvas.draw_shape(pos, visual.shape, visual.color);
          if (const Energy* energy = registry.try_get<Energy>(entity)) {
            canvas.draw_label(pos, energy_label(*energy), label_color(),
                              LabelPlacement::Center == LabelPlacement::Below
                                  ? LabelPlacement::Center
                                  : LabelPlacement::Below,
                              LabelSize::Small);
          }
        });
  };
}

}  // namespace grid
}  // namespace life_worlds
