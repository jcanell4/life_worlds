#pragma once

#include <cstdint>
#include <sstream>
#include <string>

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
struct Event {
  EventKind kind = EventKind::Generic;
  entt::entity entity = entt::null;
  entt::entity other = entt::null;
  std::string name;
  int x = 0;
  int y = 0;
  int value = 0;
  std::string detail;

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
