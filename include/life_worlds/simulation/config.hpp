#pragma once

namespace life_worlds {

// Simulation / display settings (not environment geometry).
struct SimulationConfig {
  // 0 = run until the window is closed (visual) / until interrupted.
  int tick_count = 0;
  // Stop when every active agent has EpisodeCycles >= this value (0 = off).
  // Ignored while every agent process is in eval (no training entities).
  int survive_cycles = 0;
  // How many simulation cycles make up one time unit (e.g. one year).
  int cycles_per_time_unit = 50;
  unsigned int rng_seed = 42;

  // Pixel size of one cell when drawing with raylib.
  int cell_size_px = 28;
  // Height in pixels of the status bar under the grid.
  int hud_height_px = 48;
  // Number of text rows used to lay out HUD metrics (>= 1).
  int hud_lines = 3;
  // Simulation pace while the world is being drawn (independent of display_fps).
  float ticks_per_second = 5.0f;
  // Target frame rate while drawing the world (input checked each frame).
  int display_fps = 60;
  // Target frame rate of the fast-train overlay (input cadence when not drawing).
  int fast_display_fps = 30;
  // Wall-clock budget for simulation steps between input checks in fast mode.
  float fast_slice_seconds = 1.0f / 60.0f;

  bool log_events = false;
  bool log_tick_summary = true;
};

}  // namespace life_worlds
