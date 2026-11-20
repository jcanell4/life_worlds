#include "life_worlds/environment/grid/render_layers/cell_canvas.hpp"
#include "life_worlds/environment/grid/render_layers/render_layers.hpp"
#include "life_worlds/plant/random_plant/components.hpp"
#include "life_worlds/plant/random_plant/visual.hpp"

namespace life_worlds {
namespace grid {

// Remains of eaten / cut random_plant plants. Not selectable: residues are
// not entities with a Visual, they belong to random_plant. The residue keeps
// the shape of the plant it came from; color and letter mark how it was
// removed (gray E = eaten, brown K = cut).
RenderLayer make_residue_layer(const GridEnvironment& env) {
  return [&env](const entt::registry& registry,
                const RenderContext& render_ctx) {
    const CellCanvas canvas(env, render_ctx);
    auto residues =
        registry.view<const random_plant::PlantResidue, const Position>();
    for (auto entity : residues) {
      const auto& residue =
          residues.get<const random_plant::PlantResidue>(entity);
      const auto& pos = residues.get<const Position>(entity);
      canvas.draw_shape(pos, residue.visual.shape,
                        random_plant::residue_color(residue.kind));
      canvas.draw_label(pos, random_plant::residue_label(residue.kind),
                        random_plant::residue_label_color(),
                        LabelPlacement::Center, LabelSize::Large);
    }
  };
}

}  // namespace grid
}  // namespace life_worlds
