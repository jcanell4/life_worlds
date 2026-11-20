#include "life_worlds/config_loader.hpp"

#include "life_worlds/app/grid_compose.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <utility>

namespace life_worlds {
namespace {

using json = nlohmann::json;

void fail(std::string* error_out, const std::string& message) {
  if (error_out != nullptr) {
    *error_out = message;
  }
}

SimulationConfig parse_simulation(const json& root) {
  SimulationConfig sim;
  sim.tick_count = root.value("tick_count", sim.tick_count);
  sim.survive_cycles = root.value("survive_cycles", sim.survive_cycles);
  sim.cycles_per_time_unit =
      root.value("cycles_per_time_unit", sim.cycles_per_time_unit);
  sim.rng_seed = root.value("rng_seed", sim.rng_seed);
  sim.cell_size_px = root.value("cell_size_px", sim.cell_size_px);
  sim.hud_height_px = root.value("hud_height_px", sim.hud_height_px);
  sim.hud_lines = root.value("hud_lines", sim.hud_lines);
  if (sim.hud_lines < 1) {
    sim.hud_lines = 1;
  }
  sim.ticks_per_second = root.value("ticks_per_second", sim.ticks_per_second);
  sim.display_fps = root.value("display_fps", sim.display_fps);
  sim.fast_display_fps = root.value("fast_display_fps", sim.fast_display_fps);
  sim.fast_slice_seconds =
      root.value("fast_slice_seconds", sim.fast_slice_seconds);
  if (sim.display_fps < 1) {
    sim.display_fps = 1;
  }
  if (sim.fast_display_fps < 1) {
    sim.fast_display_fps = 1;
  }
  if (sim.fast_slice_seconds <= 0.0f) {
    sim.fast_slice_seconds = 1.0f / 60.0f;
  }
  sim.log_events = root.value("log_events", sim.log_events);
  sim.log_tick_summary = root.value("log_tick_summary", sim.log_tick_summary);
  return sim;
}

}  // namespace

bool load_config(const std::string& path, LoadedConfig& out,
                 std::string* error_out) {
  try {
    std::ifstream input(path);
    if (!input) {
      fail(error_out, "cannot open config file: " + path);
      return false;
    }
    json root;
    input >> root;

    LoadedConfig loaded;
    loaded.simulation = parse_simulation(root);
    app::compose_grid_app(root, loaded);

    out = std::move(loaded);
    return true;
  } catch (const std::exception& ex) {
    fail(error_out, std::string("config error: ") + ex.what());
    return false;
  }
}

}  // namespace life_worlds
