#pragma once

namespace life_worlds {
namespace agent {

// Marker: entity is an agent that occupies a cell and appears in vision.
// Module-specific markers (ql_agent::QlAgent, c51_agent::C51Agent) sit beside it.
struct Agent {};

// Cycles survived in the current episode (eval score / train episode length).
struct EpisodeCycles {
  int value = 0;
};

// Inactive after energy depletion in eval mode. An inactive agent also loses
// its Visual, so no render layer draws it, and it no longer blocks cells.
struct Inactive {};

}  // namespace agent
}  // namespace life_worlds
