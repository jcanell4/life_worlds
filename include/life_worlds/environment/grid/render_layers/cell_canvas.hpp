#pragma once

#include "life_worlds/core/components.hpp"
#include "life_worlds/core/visual.hpp"
#include "life_worlds/environment/environment.hpp"
#include "life_worlds/environment/grid/environment.hpp"

#include <cstdint>
#include <string>

namespace life_worlds {
namespace grid {

// Where a label sits relative to the shape drawn in its cell.
enum class LabelPlacement : std::uint8_t {
  Center,  // over the shape
  Below,   // under the shape, inside the cell
};

// Label font size, relative to the cell size.
enum class LabelSize : std::uint8_t {
  Small,  // a third of the cell
  Large,  // half of the cell
};

// Cell-based drawing shared by the grid render layers: the steps a layer
// combines to draw an entity. Build one per frame; drawing on a cell outside
// the grid does nothing.
class CellCanvas {
 public:
  CellCanvas(const GridEnvironment& env, const RenderContext& render_ctx);

  // Draw `shape` centered in the cell.
  void draw_shape(Position cell, Shape shape, Rgba color) const;

  // Draw `text` horizontally centered in the cell.
  void draw_label(Position cell, const std::string& text, Rgba color,
                  LabelPlacement placement, LabelSize size) const;

 private:
  bool can_draw(Position cell) const;

  const GridEnvironment& env_;
  int cell_px_ = 0;
};

}  // namespace grid
}  // namespace life_worlds
