#pragma once

namespace life_worlds {
namespace dqn_agent {

// Marker: entity is an agent managed by DqnAgentProcess. Its energy is stored
// in the shared Energy component. It also carries agent::Agent so the grid
// treats it as an occupying agent.
struct DqnAgent {};

}  // namespace dqn_agent
}  // namespace life_worlds
