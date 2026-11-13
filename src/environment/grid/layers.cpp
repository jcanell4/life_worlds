#include "life_worlds/environment/grid/layers.hpp"

#include "life_worlds/agent/ql_agent/components.hpp"
#include "life_worlds/agent/ql_agent/visual.hpp"
#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/random_plant/components.hpp"
#include "life_worlds/plant/random_plant/visual.hpp"

#include <raylib.h>

#include <algorithm>
#include <string>

namespace life_worlds {
namespace grid {
namespace {

Color empty_fill() { return Color{232, 236, 228, 255}; }
Color grid_line() { return Color{180, 190, 176, 255}; }

void draw_plant_shape(int kind, float cx, float cy, float cell, Color color) {
  if (random_plant::uses_circle_shape(kind)) {
    DrawCircleV(Vector2{cx, cy}, cell * 0.32f, color);
  } else {
    const float half = cell * 0.30f;
    DrawRectangle(static_cast<int>(cx - half), static_cast<int>(cy - half),
                  static_cast<int>(half * 2.0f), static_cast<int>(half * 2.0f),
                  color);
  }
}

void draw_grid_layer(const GridEnvironment& env, const entt::registry&,
                     const RenderContext& render_ctx) {
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
}

void draw_residues_layer(const GridEnvironment& env,
                         const entt::registry& registry,
                         const RenderContext& render_ctx) {
  const int cell = render_ctx.display.cell_size_px;
  if (cell <= 0) {
    return;
  }
  auto residues =
      registry.view<const random_plant::PlantResidue, const Position>();
  for (auto entity : residues) {
    const auto& residue = residues.get<const random_plant::PlantResidue>(entity);
    const auto& pos = residues.get<const Position>(entity);
    if (!env.in_bounds(pos.x, pos.y)) {
      continue;
    }
    const float cx = static_cast<float>(pos.x * cell) + cell * 0.5f;
    const float cy = static_cast<float>(pos.y * cell) + cell * 0.5f;
    draw_plant_shape(static_cast<int>(residue.species), cx, cy,
                     static_cast<float>(cell),
                     random_plant::residue_color(residue.kind));
    const char* label = random_plant::residue_label(residue.kind);
    const int label_font = std::max(8, cell / 2);
    const int label_width = MeasureText(label, label_font);
    DrawText(label, static_cast<int>(cx) - label_width / 2,
             static_cast<int>(cy) - label_font / 2, label_font,
             random_plant::residue_label_color());
  }
}

void draw_plants_layer(const GridEnvironment& env,
                       const entt::registry& registry,
                       const RenderContext& render_ctx) {
  const int cell = render_ctx.display.cell_size_px;
  if (cell <= 0) {
    return;
  }
  auto plants = registry.view<const random_plant::Plant,
                              const random_plant::PlantType, const Position>();
  for (auto entity : plants) {
    const auto kind = plants.get<const random_plant::PlantType>(entity).kind;
    const auto& pos = plants.get<const Position>(entity);
    if (!env.in_bounds(pos.x, pos.y)) {
      continue;
    }
    const float cx = static_cast<float>(pos.x * cell) + cell * 0.5f;
    const float cy = static_cast<float>(pos.y * cell) + cell * 0.5f;
    draw_plant_shape(static_cast<int>(kind), cx, cy, static_cast<float>(cell),
                     random_plant::color_for_kind(kind));
  }
}

void draw_ql_agents_layer(const GridEnvironment& env,
                          const entt::registry& registry,
                          const RenderContext& render_ctx) {
  const int cell = render_ctx.display.cell_size_px;
  if (cell <= 0) {
    return;
  }
  auto agents = registry.view<const ql_agent::QlAgent, const Position,
                              const ql_agent::Energy>();
  const int energy_font = std::max(8, cell / 3);
  for (auto entity : agents) {
    if (registry.all_of<ql_agent::Inactive>(entity)) {
      continue;
    }
    const auto& pos = agents.get<const Position>(entity);
    const int energy = agents.get<const ql_agent::Energy>(entity).value;
    if (!env.in_bounds(pos.x, pos.y)) {
      continue;
    }
    const float cx = static_cast<float>(pos.x * cell) + cell * 0.5f;
    const float cy = static_cast<float>(pos.y * cell) + cell * 0.5f;
    const float s = cell * 0.28f;
    DrawTriangle(Vector2{cx, cy - s}, Vector2{cx - s, cy + s},
                 Vector2{cx + s, cy + s}, ql_agent::agent_color());

    const std::string energy_text = std::to_string(energy);
    const int text_width = MeasureText(energy_text.c_str(), energy_font);
    DrawText(energy_text.c_str(), static_cast<int>(cx) - text_width / 2,
             static_cast<int>(cy + s) + 1, energy_font,
             ql_agent::energy_text_color());
  }
}

}  // namespace

void register_default_render_layers(GridEnvironment& env) {
  env.add_render_layer([&env](const entt::registry& registry,
                              const RenderContext& ctx) {
    draw_grid_layer(env, registry, ctx);
  });
  env.add_render_layer([&env](const entt::registry& registry,
                              const RenderContext& ctx) {
    draw_residues_layer(env, registry, ctx);
  });
  env.add_render_layer([&env](const entt::registry& registry,
                              const RenderContext& ctx) {
    draw_plants_layer(env, registry, ctx);
  });
  env.add_render_layer([&env](const entt::registry& registry,
                              const RenderContext& ctx) {
    draw_ql_agents_layer(env, registry, ctx);
  });
}

}  // namespace grid
}  // namespace life_worlds
