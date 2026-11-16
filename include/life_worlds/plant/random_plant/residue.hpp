#pragma once

#include "life_worlds/plant/random_plant/components.hpp"

#include <entt/entt.hpp>

namespace life_worlds {
namespace random_plant {

// Residue lifecycle (PlantResidue entities). Every module that removes or
// spawns a plant goes through these functions instead of touching residue
// entities itself.

// Remove any residue on cell (x, y), e.g. before a living plant spawns there.
void clear_residue_at(entt::registry& registry, int x, int y);

// Mark cell (x, y) with the residue of a plant of `species` that was removed
// in the way given by `kind`; `visual` is the look of that plant. Replaces any
// residue already on the cell, so a cell holds at most one.
void leave_residue(entt::registry& registry, int x, int y, PlantKind species,
                   ResidueKind kind, const Visual& visual);

}  // namespace random_plant
}  // namespace life_worlds
