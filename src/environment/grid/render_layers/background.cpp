#include "life_worlds/environment/grid/render_layers/render_layers.hpp"

#include <raylib.h>

namespace life_worlds {
namespace grid {
namespace {

Color empty_fill() { return Color{232, 236, 228, 255}; }
Color grid_line() { return Color{180, 190, 176, 255}; }

}  // namespace

// Every cell as an empty cell: light fill and grid lines.
RenderLayer make_background_layer(const GridEnvironment& env) {
  return [&env](const entt::registry&, const RenderContext& render_ctx) {
    const int n = env.grid_size();
    const int cell = render_ctx.display.cell_size_px;
    if (n <= 0 || cell <= 0) {
      return;
    }
    for (int y = 0; y < n; ++y) {
      for (int x = 0; x < n; ++x) {
        const int px = x * cell;
        const int py = y * cell;
        DrawRectangle(px, py, cell, cell, empty_fill());
        DrawRectangleLines(px, py, cell, cell, grid_line());
      }
    }
  };
}

}  // namespace grid
}  // namespace life_worlds
