#include "life_worlds/simulation/simulation.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

// True when --console/--headless or LIFE_WORLDS_HEADLESS requests console mode.
bool wants_console(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--console") == 0 ||
        std::strcmp(argv[i], "--headless") == 0) {
      return true;
    }
  }
  if (const char* env = std::getenv("LIFE_WORLDS_HEADLESS")) {
    const std::string value(env);
    return value == "1" || value == "true";
  }
  return false;
}

// Resolve --config <path>, or default to config/random_plant_ql_1.json.
std::string config_path_from_args(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--config") == 0) {
      if (i + 1 < argc) {
        return argv[i + 1];
      }
      return {};
    }
  }
  return "config/random_plant_ql_1.json";
}

}  // namespace

// Entry point: load config and run console or visual simulation.
int main(int argc, char** argv) {
  const std::string config_path = config_path_from_args(argc, argv);
  if (config_path.empty()) {
    std::cerr << "life_worlds: --config requires a file path\n";
    return 1;
  }

  life_worlds::Simulation simulation;
  if (!simulation.init(config_path)) {
    std::cerr << "life_worlds: " << simulation.last_error() << '\n';
    return 1;
  }

  if (wants_console(argc, argv)) {
    simulation.run_console();
  } else {
    simulation.run_visual();
  }
  return 0;
}
