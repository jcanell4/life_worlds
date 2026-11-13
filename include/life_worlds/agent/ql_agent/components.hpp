#pragma once

namespace life_worlds {
namespace ql_agent {

struct QlAgent {};

struct Energy {
  int value = 100;
};

// Cycles survived in the current episode (eval score / train episode length).
struct EpisodeCycles {
  int value = 0;
};

// Inactive after energy depletion in eval mode.
struct Inactive {};

}  // namespace ql_agent
}  // namespace life_worlds
