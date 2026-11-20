#include "life_worlds/environment/grid/render_layers/cell_canvas.hpp"

#include <raylib.h>

#include <algorithm>

namespace life_worlds {
namespace grid {
namespace {

Color to_raylib(Rgba c) { return Color{c.r, c.g, c.b, c.a}; }

// Half-height of the triangle shape, as a fraction of the cell. Labels placed
// below a shape start right under it.
constexpr float kTriangleHalf = 0.28f;

}  // namespace

CellCanvas::CellCanvas(const GridEnvironment& env,
                       const RenderContext& render_ctx)
    : env_(env), cell_px_(render_ctx.display.cell_size_px) {}

bool CellCanvas::can_draw(Position cell) const {
  return cell_px_ > 0 && env_.in_bounds(cell.x, cell.y);
}

void CellCanvas::draw_shape(Position cell, Shape shape, Rgba color) const {
  if (!can_draw(cell)) {
    return;
  }
  const float size = static_cast<float>(cell_px_);
  const float cx = static_cast<float>(cell.x * cell_px_) + size * 0.5f;
  const float cy = static_cast<float>(cell.y * cell_px_) + size * 0.5f;
  const Color fill = to_raylib(color);
  switch (shape) {
    case Shape::Circle:
      DrawCircleV(Vector2{cx, cy}, size * 0.32f, fill);
      break;
    case Shape::Square: {
      const float half = size * 0.30f;
      DrawRectangle(static_cast<int>(cx - half), static_cast<int>(cy - half),
                    static_cast<int>(half * 2.0f),
                    static_cast<int>(half * 2.0f), fill);
      break;
    }
    case Shape::Triangle: {
      const float s = size * kTriangleHalf;
      DrawTriangle(Vector2{cx, cy - s}, Vector2{cx - s, cy + s},
                   Vector2{cx + s, cy + s}, fill);
      break;
    }
    case Shape::Diamond:
      DrawPoly(Vector2{cx, cy}, 4, size * 0.40f, 0.0f, fill);
      break;
    case Shape::Hexagon:
      DrawPoly(Vector2{cx, cy}, 6, size * 0.40f, 0.0f, fill);
      break;
  }
}

void CellCanvas::draw_label(Position cell, const std::string& text, Rgba color,
                            LabelPlacement placement, LabelSize size) const {
  if (!can_draw(cell)) {
    return;
  }
  const float cell_size = static_cast<float>(cell_px_);
  const float cx = static_cast<float>(cell.x * cell_px_) + cell_size * 0.5f;
  const float cy = static_cast<float>(cell.y * cell_px_) + cell_size * 0.5f;
  const int font =
      std::max(8, size == LabelSize::Large ? cell_px_ / 2 : cell_px_ / 3);
  const int width = MeasureText(text.c_str(), font);
  const int top = placement == LabelPlacement::Center
                      ? static_cast<int>(cy) - font / 2
                      : static_cast<int>(cy + cell_size * kTriangleHalf) + 1;
  DrawText(text.c_str(), static_cast<int>(cx) - width / 2, top, font,
           to_raylib(color));
}

}  // namespace grid
}  // namespace life_worlds
