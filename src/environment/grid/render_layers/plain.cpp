#include "life_worlds/environment/grid/render_layers/cell_canvas.hpp"
#include "life_worlds/environment/grid/render_layers/render_layers.hpp"

namespace life_worlds {
namespace grid {

// "plain": the entity's shape in its color, nothing else.
RenderLayer make_plain_layer(const GridEnvironment& env) {
  return [&env](const entt::registry& registry,
                const RenderContext& render_ctx) {
    const CellCanvas canvas(env, render_ctx);
    for_each_in_layer(
        registry, kPlainLayer,
        [&canvas](entt::entity, const Position& pos, const Visual& visual) {
          canvas.draw_shape(pos, visual.shape, visual.color);
        });
  };
}

}  // namespace grid
}  // namespace life_worlds
