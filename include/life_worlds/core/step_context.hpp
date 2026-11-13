#pragma once

#include "life_worlds/core/tick.hpp"
#include "life_worlds/simulation/config.hpp"
#include "life_worlds/simulation/events/event_queue.hpp"

#include <entt/entt.hpp>

#include <random>

namespace life_worlds {

class Environment;

// Shared inputs passed to every Entity::step and Environment::step call.
struct StepContext {
  entt::registry& registry;
  std::mt19937& rng;
  const SimulationConfig& simulation;
  Environment& environment;
  events::EventQueue& events;
  TickAccumulator& tick;
};

}  // namespace life_worlds
