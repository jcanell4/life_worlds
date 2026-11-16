#pragma once

#include "life_worlds/core/events/event_queue.hpp"
#include "life_worlds/core/tick.hpp"

#include <string>
#include <vector>

namespace life_worlds {
namespace events {

// Add one to the entry called `name`, creating it at the end if it is new.
inline void increment_named(std::vector<NamedCount>& counts,
                            const std::string& name) {
  for (auto& entry : counts) {
    if (entry.name == name) {
      ++entry.value;
      return;
    }
  }
  counts.push_back(NamedCount{name, 1});
}

// Derive the tick's population metrics from the Birth / Death events emitted
// during the cycle: totals plus per-name counts. Called once per tick by the
// simulation, so modules only emit events and never count them themselves.
// Names are listed in order of first appearance in the queue.
inline void tally_population(const EventQueue& queue, TickAccumulator& tick) {
  for (const auto& event : queue.events()) {
    if (event.kind == EventKind::Birth) {
      ++tick.births;
      increment_named(tick.births_by_name, event.name);
    } else if (event.kind == EventKind::Death) {
      ++tick.deaths;
      increment_named(tick.deaths_by_name, event.name);
    }
  }
}

}  // namespace events
}  // namespace life_worlds
