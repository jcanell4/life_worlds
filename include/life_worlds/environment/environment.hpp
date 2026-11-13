#pragma once

#include "life_worlds/core/step_context.hpp"
#include "life_worlds/simulation/config.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <utility>
#include <vector>

namespace life_worlds {

// Display parameters needed to draw the world (owned by SimulationConfig).
struct RenderContext {
  const SimulationConfig& display;
};

// Pixel size of the world view (excludes simulation HUD). May be non-square.
struct DisplayExtent {
  int width_px = 0;
  int height_px = 0;
};

// One drawable world layer registered by the composition root.
using RenderLayer =
    std::function<void(const entt::registry&, const RenderContext&)>;

// Abstract world: applies actions, optional passive step, and rendering.
class Environment {
 public:
  virtual ~Environment() = default;

  // Passiveive world dynamics (weather, nutrients, ...). Minimal for grid today.
  virtual void step(StepContext& ctx) = 0;

  // World viewport size in pixels for the given display settings.
  virtual DisplayExtent display_extent(const SimulationConfig& display) const = 0;

  // Append a visual plugin layer (draw order = registration order).
  void add_render_layer(RenderLayer layer) {
    render_layers_.push_back(std::move(layer));
  }

  // Draw registered layers. Call between BeginDrawing/EndDrawing.
  virtual void render(const entt::registry& registry,
                      const RenderContext& render_ctx) const {
    for (const auto& layer : render_layers_) {
      layer(registry, render_ctx);
    }
  }

  virtual void shutdown() {}

 private:
  std::vector<RenderLayer> render_layers_;
};

}  // namespace life_worlds
