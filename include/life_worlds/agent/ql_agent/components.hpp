#pragma once

namespace life_worlds {
namespace ql_agent {

// Marker: entity is an agent managed by QlAgentProcess. Its energy is stored
// in the shared Energy component (core/components.hpp), in whole units. It
// also carries agent::Agent so the grid treats it as an occupying agent.
struct QlAgent {};

}  // namespace ql_agent
}  // namespace life_worlds
