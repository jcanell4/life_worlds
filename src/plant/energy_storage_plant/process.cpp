#include "life_worlds/plant/energy_storage_plant/process.hpp"

#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/energy_storage_plant/components.hpp"
#include "life_worlds/plant/energy_storage_plant/visual.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace life_worlds {
namespace energy_storage_plant {
namespace {

using Cell = std::pair<int, int>;

// Orthogonal neighbours (up, down, left, right).
constexpr std::array<Cell, 4> kNeighbours{{{0, -1}, {0, 1}, {-1, 0}, {1, 0}}};

// Create a living plant at (x, y) belonging to the given lineage.
entt::entity spawn_plant(entt::registry& registry, PlantKind kind,
                         const PlantTypeConfig& type, int x, int y,
                         std::uint32_t lineage_id, double energy) {
  const entt::entity entity = registry.create();
  registry.emplace<EnergyPlant>(entity);
  registry.emplace<plant::PlantTypeIndex>(entity, plant::PlantTypeIndex{kind});
  registry.emplace<Position>(entity, Position{x, y});
  registry.emplace<StoredEnergy>(
      entity, StoredEnergy{energy, type.maximum_energy_limit});
  registry.emplace<Lineage>(entity, Lineage{lineage_id});
  registry.emplace<Visual>(
      entity, resolve_visual(type.visual, default_shape(),
                             lineage_color(lineage_id)));
  return entity;
}

// Cells the port reports as free, minus the cells already holding one of our
// plants (the port does not know about this module yet).
std::set<Cell> free_cell_set(const entt::registry& registry,
                             const OccupancyPort& occupancy) {
  std::set<Cell> free;
  for (const auto& cell : occupancy.free_cells(registry)) {
    free.insert(cell);
  }
  auto view = registry.view<const EnergyPlant, const Position>();
  for (auto entity : view) {
    const auto& pos = view.get<const Position>(entity);
    free.erase(Cell{pos.x, pos.y});
  }
  return free;
}

// First step: place initial_count founders per species, one lineage each.
// Emits a Birth event (without parent) for each founder.
void seed_founders(entt::registry& registry,
                   const EnergyStoragePlantConfig& config,
                   const OccupancyPort& occupancy,
                   std::uint32_t& next_lineage_id, std::mt19937& rng,
                   events::EventQueue& event_queue) {
  const auto free_set = free_cell_set(registry, occupancy);
  std::vector<Cell> free(free_set.begin(), free_set.end());
  std::shuffle(free.begin(), free.end(), rng);

  for (std::size_t i = 0; i < config.plant_types.size(); ++i) {
    const auto& type = config.plant_types[i];
    const auto kind = static_cast<PlantKind>(i);
    for (int n = 0; n < type.initial_count && !free.empty(); ++n) {
      const Cell cell = free.back();
      free.pop_back();
      const double energy =
          std::clamp(type.initial_energy, 0.0, type.maximum_energy_limit);
      const entt::entity entity =
          spawn_plant(registry, kind, type, cell.first, cell.second,
                      next_lineage_id++, energy);
      event_queue.emit(events::Event::birth(entity, entt::null, type.name,
                                            cell.first, cell.second));
    }
  }
}

// Accumulate energy and split full plants into adjacent free cells.
// Emits a Birth event for each new plant, with the splitting plant as parent.
void grow_plants(entt::registry& registry,
                 const EnergyStoragePlantConfig& config,
                 const OccupancyPort& occupancy, std::mt19937& rng,
                 events::EventQueue& event_queue) {
  auto free = free_cell_set(registry, occupancy);

  // Cells per lineage, kept up to date as plants split this cycle.
  std::unordered_map<std::uint32_t, int> lineage_cells;
  std::vector<entt::entity> plants;
  {
    auto view = registry.view<const EnergyPlant, const Lineage>();
    for (auto entity : view) {
      ++lineage_cells[view.get<const Lineage>(entity).id];
      plants.push_back(entity);
    }
  }
  // Random order so no plant always gets the free cells first.
  std::shuffle(plants.begin(), plants.end(), rng);

  for (const entt::entity entity : plants) {
    const auto& pos = registry.get<Position>(entity);
    const PlantKind kind = registry.get<plant::PlantTypeIndex>(entity).kind;
    const auto& type = config.plant_types[static_cast<std::size_t>(kind)];
    const std::uint32_t lineage_id = registry.get<Lineage>(entity).id;

    std::vector<Cell> open;
    for (const auto& [dx, dy] : kNeighbours) {
      const Cell cell{pos.x + dx, pos.y + dy};
      if (free.count(cell) != 0) {
        open.push_back(cell);
      }
    }
    // No free neighbour: the plant neither grows nor accumulates energy.
    if (open.empty()) {
      continue;
    }

    auto& energy = registry.get<StoredEnergy>(entity);
    energy.value = std::min(energy.max, energy.value + type.growing_rate);

    // Lineage at its cell cap: keeps filling up to the max, never splits.
    const bool lineage_full =
        lineage_cells[lineage_id] >= type.maximum_cell_occupancy;
    if (lineage_full || energy.value < energy.max) {
      continue;
    }

    std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);
    const Cell cell = open[pick(rng)];
    const double transferred = energy.value * type.split_fraction;
    energy.value -= transferred;

    const entt::entity child = spawn_plant(registry, kind, type, cell.first,
                                           cell.second, lineage_id, transferred);
    free.erase(cell);
    ++lineage_cells[lineage_id];
    event_queue.emit(events::Event::birth(child, entity, type.name, cell.first,
                                          cell.second));
  }
}

}  // namespace

// Construct the process with species config and an occupancy port.
EnergyStoragePlantProcess::EnergyStoragePlantProcess(
    EnergyStoragePlantConfig config, OccupancyPort occupancy)
    : config_(std::move(config)), occupancy_(std::move(occupancy)) {
  if (!occupancy_.free_cells) {
    throw std::runtime_error(
        "energy_storage_plant requires OccupancyPort::free_cells");
  }
}

// One cycle: place founders on the first step, then grow / split.
// Births are reported as events; the simulation tallies them.
void EnergyStoragePlantProcess::step(StepContext& ctx) {
  if (!seeded_) {
    seed_founders(ctx.registry, config_, occupancy_, next_lineage_id_, ctx.rng,
                  ctx.events);
    seeded_ = true;
  } else {
    grow_plants(ctx.registry, config_, occupancy_, ctx.rng, ctx.events);
  }

  plant::publish_alive_by_name<EnergyPlant>(ctx.registry, config_.plant_types,
                                            ctx.tick);
}

}  // namespace energy_storage_plant
}  // namespace life_worlds
