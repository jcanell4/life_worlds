#pragma once

#include "life_worlds/core/tick.hpp"
#include "life_worlds/plant/components.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <vector>

namespace life_worlds {
namespace plant {

// Constants and helpers shared by every plant module. A plant module tags its
// living plants with a marker component and stores the species index in the
// shared PlantTypeIndex component.

// Soft upper bound on species count for a single plant entity module.
inline constexpr std::size_t kMaxPlantKinds = 32;

// Living plants per species of the module tagged with Marker, indexed by
// PlantKind.
template <typename Marker>
std::vector<int> count_alive_by_kind(const entt::registry& registry,
                                     std::size_t kind_count) {
  std::vector<int> alive(kind_count, 0);
  auto view = registry.view<const Marker, const PlantTypeIndex>();
  for (auto entity : view) {
    const auto kind = static_cast<std::size_t>(
        view.template get<const PlantTypeIndex>(entity).kind);
    if (kind < kind_count) {
      ++alive[kind];
    }
  }
  return alive;
}

// Append the living count of every species to tick.alive_by_name, in
// `plant_types` order. TypeConfig must expose a `name` member.
template <typename Marker, typename TypeConfig>
void publish_alive_by_name(const entt::registry& registry,
                           const std::vector<TypeConfig>& plant_types,
                           TickAccumulator& tick) {
  const auto alive = count_alive_by_kind<Marker>(registry, plant_types.size());
  for (std::size_t i = 0; i < plant_types.size(); ++i) {
    tick.alive_by_name.push_back(NamedCount{plant_types[i].name, alive[i]});
  }
}

}  // namespace plant
}  // namespace life_worlds
