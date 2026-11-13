#pragma once

#include "life_worlds/simulation/events/event.hpp"

#include <utility>
#include <vector>

namespace life_worlds {
namespace events {

// Per-tick event buffer shared through StepContext.
class EventQueue {
 public:
  // Append an event produced by environment or an entity module.
  void emit(Event event) { events_.push_back(std::move(event)); }

  // Remove all events (start of each simulation cycle).
  void clear() { events_.clear(); }

  const std::vector<Event>& events() const { return events_; }
  std::vector<Event>& events() { return events_; }

 private:
  std::vector<Event> events_;
};

}  // namespace events
}  // namespace life_worlds
