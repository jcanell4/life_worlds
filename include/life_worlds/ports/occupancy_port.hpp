#pragma once

#include <entt/entt.hpp>

#include <functional>
#include <utility>
#include <vector>

namespace life_worlds {

// World occupancy queries in integer cells (grid-capable apps).
struct OccupancyPort {
  std::function<std::vector<std::pair<int, int>>(const entt::registry&)>
      free_cells;
};

}  // namespace life_worlds
