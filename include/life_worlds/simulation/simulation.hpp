#pragma once

#include "life_worlds/core/tick.hpp"
#include "life_worlds/entity/entity.hpp"
#include "life_worlds/environment/environment.hpp"
#include "life_worlds/simulation/config.hpp"
#include "life_worlds/simulation/events/event_queue.hpp"

#include <entt/entt.hpp>

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace life_worlds {

// Owns config, environment, entities, ECS registry, RNG, and run loops.
class Simulation {
 public:
  Simulation() = default;

  // Load JSON config, build environment + entities, reset world state.
  bool init(const std::string& config_path);

  const std::string& last_error() const { return last_error_; }

  // Advance one cycle: clear events → env.step → entity.steps.
  const TickAccumulator& step();

  void run_console();
  void run_visual();

  const SimulationConfig& config() const { return config_; }
  const Environment& environment() const { return *environment_; }
  const entt::registry& registry() const { return registry_; }
  int tick_index() const { return tick_index_; }
  const TickAccumulator& last_tick() const { return last_tick_; }
  int total_births() const { return total_births_; }
  int total_deaths() const { return total_deaths_; }

 private:
  void log_tick(const TickAccumulator& tick) const;
  void log_summary() const;

  SimulationConfig config_{};
  std::unique_ptr<Environment> environment_;
  std::vector<std::unique_ptr<Entity>> entities_;
  entt::registry registry_{};
  std::mt19937 rng_{};
  events::EventQueue events_{};
  TickAccumulator last_tick_{};
  int total_births_ = 0;
  int total_deaths_ = 0;
  int tick_index_ = 0;
  std::string last_error_;
};

}  // namespace life_worlds
