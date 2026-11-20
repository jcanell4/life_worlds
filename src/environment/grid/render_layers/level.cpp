#include "life_worlds/environment/grid/render_layers/cell_canvas.hpp"
#include "life_worlds/environment/grid/render_layers/render_layers.hpp"

namespace life_worlds {
namespace grid {

// "level": the shape gets brighter as Energy fills up (value / max) and shows
// the value over it. An entity without Energy is drawn as in "plain".
RenderLayer make_level_layer(const GridEnvironment& env) {
  return [&env](const entt::registry& registry,
                const RenderContext& render_ctx) {
    const CellCanvas canvas(env, render_ctx);
    for_each_in_layer(
        registry, kLevelLayer,
        [&](entt::entity entity, const Position& pos, const Visual& visual) {
          const Energy* energy = registry.try_get<Energy>(entity);
          if (energy == nullptr) {
            canvas.draw_shape(pos, visual.shape, visual.color);
            return;
          }
          const double fraction =
              energy->max > 0.0 ? energy->value / energy->max : 0.0;
          const Rgba fill = shade_by_fraction(visual.color, fraction);
          canvas.draw_shape(pos, visual.shape, fill);
          canvas.draw_label(pos, energy_label(*energy),
                            contrast_text_color(fill), LabelPlacement::Center,
                            LabelSize::Small);
        });
  };
}

}  // namespace grid
}  // namespace life_worlds
