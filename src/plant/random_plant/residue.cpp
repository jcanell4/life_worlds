#include "life_worlds/plant/random_plant/residue.hpp"

#include "life_worlds/core/components.hpp"

#include <vector>

namespace life_worlds {
namespace random_plant {

// Remove any residue on cell (x, y).
void clear_residue_at(entt::registry& registry, int x, int y) {
  std::vector<entt::entity> to_destroy;
  auto view = registry.view<const PlantResidue, const Position>();
  for (auto entity : view) {
    const auto& pos = view.get<const Position>(entity);
    if (pos.x == x && pos.y == y) {
      to_destroy.push_back(entity);
    }
  }
  for (auto entity : to_destroy) {
    registry.destroy(entity);
  }
}

// Replace whatever residue is on cell (x, y) with a new one.
void leave_residue(entt::registry& registry, int x, int y, PlantKind species,
                   ResidueKind kind, const Visual& visual) {
  clear_residue_at(registry, x, y);
  const entt::entity residue = registry.create();
  registry.emplace<PlantResidue>(residue, PlantResidue{kind, species, visual});
  registry.emplace<Position>(residue, Position{x, y});
}

}  // namespace random_plant
}  // namespace life_worlds
