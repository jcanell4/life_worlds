#pragma once

namespace life_worlds {
namespace c51_agent {

// Marker: entity is an agent managed by C51AgentProcess. Its energy is stored
// in the shared Energy component. It also carries agent::Agent so the grid
// treats it as an occupying agent.
struct C51Agent {};

}  // namespace c51_agent
}  // namespace life_worlds
