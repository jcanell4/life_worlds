#pragma once

#include "life_worlds/core/step_context.hpp"

namespace life_worlds {

// Pluggable simulation participant (plants, QL agents, ...).
class Entity {
 public:
  virtual ~Entity() = default;

  // Advance this subsystem by one simulation cycle.
  virtual void step(StepContext& ctx) = 0;

  // True for RL agent processes in train mode (enables survive_cycles stop).
  virtual bool is_training() const { return false; }

  // Called when the simulation run ends (save models, flush scores, ...).
  virtual void shutdown() {}
};

}  // namespace life_worlds
