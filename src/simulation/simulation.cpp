#include "life_worlds/simulation/simulation.hpp"

#include "life_worlds/config_loader.hpp"

#include <raylib.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace life_worlds {
namespace {

// Sum of all alive_by_name entries for HUD / console summaries.
int total_alive(const TickAccumulator& tick) {
  int sum = 0;
  for (const auto& entry : tick.alive_by_name) {
    sum += entry.value;
  }
  return sum;
}

// Flatten tick metrics into short HUD tokens (tick, alive, energy, harvest).
std::vector<std::string> build_hud_tokens(int tick_index,
                                          const TickAccumulator& tick) {
  std::vector<std::string> tokens;
  tokens.push_back("tick " + std::to_string(tick_index));
  tokens.push_back("alive " + std::to_string(total_alive(tick)));
  for (const auto& entry : tick.alive_by_name) {
    tokens.push_back(entry.name + " " + std::to_string(entry.value));
  }
  for (const auto& entry : tick.energy_by_agent) {
    tokens.push_back(entry.name + " e=" + std::to_string(entry.value));
  }
  for (const auto& entry : tick.eaten_by_name) {
    tokens.push_back("eat_" + entry.name + "=" + std::to_string(entry.value));
  }
  for (const auto& entry : tick.cut_by_name) {
    tokens.push_back("cut_" + entry.name + "=" + std::to_string(entry.value));
  }
  return tokens;
}

// Spread tokens across line_count rows as evenly as possible (by token count).
std::vector<std::string> distribute_hud_lines(
    const std::vector<std::string>& tokens, int line_count) {
  const int lines = std::max(1, line_count);
  std::vector<std::string> out(static_cast<std::size_t>(lines));
  if (tokens.empty()) {
    return out;
  }
  const int n = static_cast<int>(tokens.size());
  for (int i = 0; i < lines; ++i) {
    const int start = i * n / lines;
    const int end = (i + 1) * n / lines;
    std::string line;
    for (int t = start; t < end; ++t) {
      if (!line.empty()) {
        line += "  ";
      }
      line += tokens[static_cast<std::size_t>(t)];
    }
    out[static_cast<std::size_t>(i)] = std::move(line);
  }
  return out;
}

}  // namespace

// Load config, own environment + entities, reset registry / RNG / counters.
bool Simulation::init(const std::string& config_path) {
  LoadedConfig loaded;
  if (!load_config(config_path, loaded, &last_error_)) {
    return false;
  }

  config_ = std::move(loaded.simulation);
  environment_ = std::move(loaded.environment);
  entities_ = std::move(loaded.entities);
  registry_ = entt::registry{};
  rng_ = std::mt19937{config_.rng_seed};
  events_.clear();
  last_tick_.clear();
  total_births_ = 0;
  total_deaths_ = 0;
  tick_index_ = 0;
  last_error_.clear();
  return true;
}

// Run environment then every Entity once; accumulate births/deaths.
const TickAccumulator& Simulation::step() {
  ++tick_index_;
  last_tick_.clear();
  events_.clear();
  StepContext ctx{registry_, rng_, config_, *environment_, events_, last_tick_};
  environment_->step(ctx);
  for (auto& entity : entities_) {
    entity->step(ctx);
  }
  total_births_ += last_tick_.births;
  total_deaths_ += last_tick_.deaths;
  log_tick(last_tick_);
  return last_tick_;
}

// Optionally print tick summary and typed event lines to stdout.
void Simulation::log_tick(const TickAccumulator& tick) const {
  if (config_.log_tick_summary) {
    std::cout << "tick " << tick_index_ << ": births=" << tick.births
              << " deaths=" << tick.deaths << " alive=" << total_alive(tick);
    for (const auto& entry : tick.alive_by_name) {
      std::cout << "  " << entry.name << "=" << entry.value;
    }
    for (const auto& entry : tick.energy_by_agent) {
      std::cout << "  " << entry.name << "_e=" << entry.value;
    }
    for (const auto& entry : tick.eaten_by_name) {
      std::cout << "  eat_" << entry.name << "=" << entry.value;
    }
    for (const auto& entry : tick.cut_by_name) {
      std::cout << "  cut_" << entry.name << "=" << entry.value;
    }
    std::cout << '\n';
  }

  if (!config_.log_events) {
    return;
  }
  for (const auto& event : events_.events()) {
    std::cout << event.to_string() << '\n';
  }
}

// Print end-of-run totals (ticks, births/deaths, harvest).
void Simulation::log_summary() const {
  std::cout << "\n=== summary ===\n";
  std::cout << "ticks=" << tick_index_ << " total_births=" << total_births_
            << " total_deaths=" << total_deaths_ << '\n';
  for (const auto& entry : last_tick_.alive_by_name) {
    std::cout << "  " << entry.name << ": alive=" << entry.value << '\n';
  }
  for (const auto& entry : last_tick_.eaten_by_name) {
    std::cout << "  eaten_" << entry.name << "=" << entry.value << '\n';
  }
  for (const auto& entry : last_tick_.cut_by_name) {
    std::cout << "  cut_" << entry.name << "=" << entry.value << '\n';
  }
}

// Headless loop: step until tick limit, then shutdown entities/environment.
void Simulation::run_console() {
  const int limit = config_.tick_count > 0 ? config_.tick_count : 40;
  const DisplayExtent extent = environment_->display_extent(config_);
  std::cout << "life_worlds — console mode\n";
  std::cout << "world_px=" << extent.width_px << "x" << extent.height_px
            << " ticks=" << limit << " seed=" << config_.rng_seed << '\n';

  for (int i = 0; i < limit; ++i) {
    step();
  }
  for (auto& entity : entities_) {
    entity->shutdown();
  }
  environment_->shutdown();
  log_summary();
}

// Open a raylib window, pace ticks, draw via environment.render, fast-train toggle.
void Simulation::run_visual() {
  const DisplayExtent extent = environment_->display_extent(config_);
  const int world_w = std::max(1, extent.width_px);
  const int world_h = std::max(1, extent.height_px);
  const int hud_lines = std::max(1, config_.hud_lines);
  const int hud = std::max(16 * hud_lines, config_.hud_height_px);
  const int line_height = std::max(12, hud / hud_lines);
  const int hud_font = std::max(8, std::min(18, line_height - 4));
  const int hud_pad_x = std::max(4, config_.cell_size_px / 4);
  const int hud_pad_y = std::max(2, (line_height - hud_font) / 2);
  const int width = world_w;
  const int height = world_h + hud;
  const int display_fps = std::max(1, config_.display_fps);
  const int fast_display_fps = std::max(1, config_.fast_display_fps);
  const double fast_slice =
      std::max(1e-4, static_cast<double>(config_.fast_slice_seconds));
  // Cap catch-up work so a large accumulator cannot starve input/render.
  const double visual_slice = 1.0 / static_cast<double>(display_fps);

  SetTraceLogLevel(LOG_WARNING);
  InitWindow(width, height, "life_worlds");
  SetTargetFPS(display_fps);

  std::cout << "life_worlds — visual mode (raylib)\n";
  std::cout << "world_px=" << world_w << "x" << world_h
            << " cell_px=" << config_.cell_size_px << " hud_px=" << hud
            << " hud_lines=" << hud_lines
            << " ticks_per_second=" << config_.ticks_per_second
            << " display_fps=" << display_fps
            << " fast_display_fps=" << fast_display_fps
            << " fast_slice_s=" << fast_slice
            << " seed=" << config_.rng_seed << '\n';
  std::cout << "Close the window or press Esc to stop.\n";
  std::cout << "F8 / F12 / Ctrl+P toggles fast training (skips world rendering).\n"
            << std::flush;

  float accumulator = 0.0f;
  const float interval =
      config_.ticks_per_second > 0.0f ? 1.0f / config_.ticks_per_second : 0.2f;
  std::vector<std::string> hud_rows =
      distribute_hud_lines(build_hud_tokens(0, TickAccumulator{}), hud_lines);
  bool skip_render = false;
  bool toggle_was_down = false;
  const RenderContext render_ctx{config_};

  // True while any fast-toggle chord is held (edge detection is separate).
  const auto toggle_chord_down = []() {
    return IsKeyDown(KEY_F8) || IsKeyDown(KEY_F12) ||
           ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
            IsKeyDown(KEY_P));
  };

  // Rising edge of the toggle chord (works across mid-frame PollInputEvents).
  const auto fast_toggle_edge = [&]() {
    const bool down = toggle_chord_down();
    const bool pressed = down && !toggle_was_down;
    toggle_was_down = down;
    return pressed;
  };

  const auto apply_fast_toggle = [&]() {
    skip_render = !skip_render;
    // Fast mode must not sleep in EndDrawing: use uncapped FPS and draw rarely.
    SetTargetFPS(skip_render ? 0 : display_fps);
    accumulator = 0.0f;
    std::cout << (skip_render ? "fast training ON (render skipped)\n"
                              : "fast training OFF (render enabled)\n")
              << std::flush;
  };

  const double overlay_period = 1.0 / static_cast<double>(fast_display_fps);
  double next_overlay_time = GetTime();

  while (!WindowShouldClose()) {
    if (fast_toggle_edge()) {
      apply_fast_toggle();
    }

    if (skip_render) {
      // Spend wall time on steps; only pause briefly to poll input.
      const double slice_start = GetTime();
      while ((GetTime() - slice_start) < fast_slice) {
        if (config_.tick_count > 0 && tick_index_ >= config_.tick_count) {
          break;
        }
        step();
      }

      PollInputEvents();
      if (WindowShouldClose()) {
        break;
      }
      if (fast_toggle_edge()) {
        apply_fast_toggle();
      }

      // Optional lightweight overlay; EndDrawing does not wait (target FPS 0).
      if (skip_render && GetTime() >= next_overlay_time) {
        hud_rows = distribute_hud_lines(
            build_hud_tokens(tick_index_, last_tick_), hud_lines);
        BeginDrawing();
        ClearBackground(Color{24, 28, 24, 255});
        DrawText("FAST TRAIN (F8 / F12 / Ctrl+P to restore view)", hud_pad_x,
                 world_h / 2, 20, Color{220, 230, 220, 255});
        DrawText(("tick " + std::to_string(tick_index_)).c_str(), hud_pad_x,
                 world_h / 2 + 28, 18, Color{180, 200, 180, 255});
        EndDrawing();
        next_overlay_time = GetTime() + overlay_period;
      }
    } else {
      accumulator += GetFrameTime();
      const double slice_start = GetTime();

      while (accumulator >= interval) {
        // Keep display/input responsive even after a hitch (large accumulator).
        if ((GetTime() - slice_start) >= visual_slice) {
          break;
        }
        accumulator -= interval;
        if (config_.tick_count > 0 && tick_index_ >= config_.tick_count) {
          break;
        }
        const TickAccumulator& tick = step();
        hud_rows = distribute_hud_lines(build_hud_tokens(tick_index_, tick),
                                        hud_lines);

        PollInputEvents();
        if (WindowShouldClose()) {
          break;
        }
        if (fast_toggle_edge()) {
          apply_fast_toggle();
          break;
        }
      }

      if (!skip_render) {
        BeginDrawing();
        ClearBackground(Color{40, 44, 40, 255});
        environment_->render(registry_, render_ctx);
        DrawRectangle(0, world_h, width, hud, Color{32, 36, 32, 255});
        for (int i = 0; i < hud_lines; ++i) {
          const std::string& row = hud_rows[static_cast<std::size_t>(i)];
          if (row.empty()) {
            continue;
          }
          DrawText(row.c_str(), hud_pad_x,
                   world_h + i * line_height + hud_pad_y, hud_font,
                   Color{220, 230, 220, 255});
        }
        EndDrawing();
      }
    }

    if (config_.tick_count > 0 && tick_index_ >= config_.tick_count) {
      break;
    }
  }

  CloseWindow();
  for (auto& entity : entities_) {
    entity->shutdown();
  }
  environment_->shutdown();
  log_summary();
}

}  // namespace life_worlds
