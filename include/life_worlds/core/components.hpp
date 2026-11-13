#pragma once

namespace life_worlds {

// Cell coordinates on the N×N world grid.
struct Position {
  int x = 0;
  int y = 0;
};

// Age measured in simulation cycles.
struct Age {
  int cycles = 0;
};

}  // namespace life_worlds
