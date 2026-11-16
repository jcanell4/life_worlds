#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include <entt/entt.hpp>

namespace life_worlds {
namespace events {

enum class EventKind {
  Birth,
  Death,
  Move,
  Eat,
  Cut,
  AgentReset,
  AgentFinished,
  Generic,
};

// Typed simulation event; modules emit, simulation logs / future env reacts.
// This is the single vocabulary for "something happened": modules emit it
// directly instead of keeping private record structs that mirror it.
struct Event {
  EventKind kind = EventKind::Generic;
  entt::entity entity = entt::null;
  entt::entity other = entt::null;
  std::string name;
  int x = 0;
  int y = 0;
  int value = 0;
  std::string detail;

  // `entity` of species / type `name` was born at (x, y). `parent` is
  // entt::null for individuals that were seeded rather than reproduced.
  static Event birth(entt::entity entity, entt::entity parent, std::string name,
                     int x, int y) {
    return Event{EventKind::Birth, entity, parent, std::move(name), x, y, 0, {}};
  }

  // `entity` of species / type `name` died at (x, y). The age in cycles goes
  // into `value` and the age stage into `detail`.
  static Event death(entt::entity entity, std::string name, int x, int y,
                     int age_cycles, int age_stage) {
    return Event{EventKind::Death, entity, entt::null, std::move(name), x, y,
                 age_cycles, std::to_string(age_stage)};
  }

  // Format a single log line for console / file sinks.
  std::string to_string() const {
    std::ostringstream line;
    switch (kind) {
      case EventKind::Birth:
        line << "  + birth id=" << static_cast<std::uint32_t>(entity)
             << " type=" << name << " cell=(" << x << "," << y << ")";
        if (other != entt::null) {
          line << " parent=" << static_cast<std::uint32_t>(other);
        }
        break;
      case EventKind::Death:
        line << "  - death id=" << static_cast<std::uint32_t>(entity)
             << " type=" << name << " age_cycles=" << value
             << " stage=" << detail << " cell=(" << x << "," << y << ")";
        break;
      case EventKind::Eat:
      case EventKind::Cut:
      case EventKind::Move:
      case EventKind::AgentReset:
      case EventKind::AgentFinished:
      case EventKind::Generic:
        line << "  " << detail;
        if (detail.empty()) {
          line << name;
        }
        break;
    }
    return line.str();
  }
};

}  // namespace events
}  // namespace life_worlds
