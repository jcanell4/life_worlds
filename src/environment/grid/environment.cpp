#include "life_worlds/environment/grid/environment.hpp"

#include "life_worlds/agent/ql_agent/components.hpp"
#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/random_plant/components.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace life_worlds {
namespace grid {
namespace {

constexpr std::array<std::pair<int, int>, kDirectionCount> kDelta{{
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
}};

int clamp_energy(int value, int energy_max) {
  return std::clamp(value, 0, std::max(1, energy_max));
}

void leave_residue(entt::registry& registry, int x, int y,
                   random_plant::PlantKind species,
                   random_plant::ResidueKind kind) {
  std::vector<entt::entity> to_destroy;
  auto view = registry.view<const random_plant::PlantResidue, const Position>();
  for (auto entity : view) {
    const auto& pos = view.get<const Position>(entity);
    if (pos.x == x && pos.y == y) {
      to_destroy.push_back(entity);
    }
  }
  for (auto entity : to_destroy) {
    registry.destroy(entity);
  }

  const entt::entity residue = registry.create();
  registry.emplace<random_plant::PlantResidue>(
      residue, random_plant::PlantResidue{kind, species});
  registry.emplace<Position>(residue, Position{x, y});
}

}  // namespace

GridEnvironment::GridEnvironment(GridConfig config)
    : config_(std::move(config)) {}

void GridEnvironment::step(StepContext& /*ctx*/) {
  // Passiveive grid dynamics reserved for future weather / nutrients.
}

// Square viewport: N cells × cell_size_px on each side.
DisplayExtent GridEnvironment::display_extent(
    const SimulationConfig& display) const {
  const int cell = std::max(4, display.cell_size_px);
  const int n = std::max(1, config_.grid_size);
  const int side = n * cell;
  return DisplayExtent{side, side};
}

bool GridEnvironment::in_bounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < config_.grid_size && y < config_.grid_size;
}

entt::entity GridEnvironment::plant_at(const entt::registry& registry, int x,
                                       int y) const {
  auto view = registry.view<const random_plant::Plant, const Position>();
  for (auto entity : view) {
    const auto& pos = view.get<const Position>(entity);
    if (pos.x == x && pos.y == y) {
      return entity;
    }
  }
  return entt::null;
}

bool GridEnvironment::cell_has_agent(const entt::registry& registry, int x,
                                     int y, entt::entity self) const {
  auto view = registry.view<const ql_agent::QlAgent, const Position>();
  for (auto entity : view) {
    if (entity == self || registry.all_of<ql_agent::Inactive>(entity)) {
      continue;
    }
    const auto& pos = view.get<const Position>(entity);
    if (pos.x == x && pos.y == y) {
      return true;
    }
  }
  return false;
}

bool GridEnvironment::cell_blocked(const entt::registry& registry, int x, int y,
                                   entt::entity self) const {
  return cell_has_agent(registry, x, y, self) ||
         plant_at(registry, x, y) != entt::null;
}

entt::entity GridEnvironment::adjacent_plant(const entt::registry& registry,
                                             int x, int y) const {
  for (const auto& [dx, dy] : kDelta) {
    const entt::entity plant = plant_at(registry, x + dx, y + dy);
    if (plant != entt::null) {
      return plant;
    }
  }
  return entt::null;
}

std::vector<std::pair<int, int>> GridEnvironment::free_cells(
    const entt::registry& registry) const {
  std::vector<std::pair<int, int>> free;
  const int n = config_.grid_size;
  if (n <= 0) {
    return free;
  }
  free.reserve(static_cast<std::size_t>(n * n));
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      if (!cell_blocked(registry, x, y, entt::null)) {
        free.emplace_back(x, y);
      }
    }
  }
  return free;
}

ActionOutcome GridEnvironment::apply_action(entt::registry& registry,
                                            entt::entity agent, Action action,
                                            const EatParams& eat,
                                            events::EventQueue& events) const {
  ActionOutcome result;
  auto& pos = registry.get<Position>(agent);
  auto& energy = registry.get<ql_agent::Energy>(agent);
  result.energy_after = energy.value;

  switch (action) {
    case Action::Up:
    case Action::Down:
    case Action::Left:
    case Action::Right: {
      const auto [dx, dy] =
          kDelta[static_cast<std::size_t>(static_cast<int>(action))];
      const int nx = pos.x + dx;
      const int ny = pos.y + dy;
      if (in_bounds(nx, ny) && !cell_blocked(registry, nx, ny, agent)) {
        pos.x = nx;
        pos.y = ny;
        result.moved = true;
        std::ostringstream detail;
        detail << "ql id=" << static_cast<std::uint32_t>(agent) << " move to ("
               << nx << "," << ny << ")";
        events.emit(events::Event{events::EventKind::Move, agent, entt::null, {},
                                  nx, ny, 0, detail.str()});
      }
      break;
    }
    case Action::Eat: {
      const entt::entity plant = adjacent_plant(registry, pos.x, pos.y);
      if (plant == entt::null ||
          !registry.all_of<random_plant::SpeciesName>(plant) ||
          !registry.all_of<random_plant::PlantType, Position>(plant)) {
        break;
      }
      const std::string name =
          registry.get<random_plant::SpeciesName>(plant).name;
      const auto species = registry.get<random_plant::PlantType>(plant).kind;
      const auto plant_pos = registry.get<Position>(plant);
      int delta = 0;
      if (name == "herb") {
        delta = static_cast<int>(
            std::lround(eat.herb_energy_delta * eat.herb_ingestion));
      } else if (name == "shrub") {
        delta = static_cast<int>(
            std::lround(eat.shrub_energy_delta * eat.shrub_ingestion));
      } else {
        break;
      }
      energy.value = clamp_energy(energy.value + delta, eat.energy_max);
      result.food_reward = static_cast<double>(delta);
      result.species_name = name;
      result.eaten = true;
      result.detail = "eat " + name;
      registry.destroy(plant);
      leave_residue(registry, plant_pos.x, plant_pos.y, species,
                    random_plant::ResidueKind::Eaten);
      std::ostringstream detail;
      detail << "ql id=" << static_cast<std::uint32_t>(agent) << " eat " << name
             << " energy=" << energy.value;
      events.emit(events::Event{events::EventKind::Eat, agent, entt::null, name,
                                plant_pos.x, plant_pos.y, energy.value,
                                detail.str()});
      break;
    }
    case Action::Cut: {
      const entt::entity plant = adjacent_plant(registry, pos.x, pos.y);
      if (plant == entt::null ||
          !registry.all_of<random_plant::PlantType, Position>(plant)) {
        break;
      }
      const auto species = registry.get<random_plant::PlantType>(plant).kind;
      const auto plant_pos = registry.get<Position>(plant);
      result.species_name =
          registry.all_of<random_plant::SpeciesName>(plant)
              ? registry.get<random_plant::SpeciesName>(plant).name
              : "plant";
      result.cut = true;
      result.detail = "cut " + result.species_name;
      registry.destroy(plant);
      leave_residue(registry, plant_pos.x, plant_pos.y, species,
                    random_plant::ResidueKind::Cut);
      std::ostringstream detail;
      detail << "ql id=" << static_cast<std::uint32_t>(agent) << " cut "
             << result.species_name;
      events.emit(events::Event{events::EventKind::Cut, agent, entt::null,
                                result.species_name, plant_pos.x, plant_pos.y, 0,
                                detail.str()});
      break;
    }
  }

  result.energy_after = energy.value;
  return result;
}

void GridEnvironment::reset_agent_position(entt::registry& registry,
                                           entt::entity agent,
                                           std::mt19937& rng) const {
  auto free = free_cells(registry);
  const auto& pos = registry.get<Position>(agent);
  free.emplace_back(pos.x, pos.y);
  if (free.empty()) {
    return;
  }
  std::uniform_int_distribution<std::size_t> pick(0, free.size() - 1);
  const auto cell = free[pick(rng)];
  registry.get<Position>(agent) = Position{cell.first, cell.second};
}

}  // namespace grid
}  // namespace life_worlds
