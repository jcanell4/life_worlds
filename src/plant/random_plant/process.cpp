#include "life_worlds/plant/random_plant/process.hpp"

#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/random_plant/components.hpp"
#include "life_worlds/plant/random_plant/visual.hpp"
#include "life_worlds/plant/random_plant/residue.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace life_worlds {
namespace random_plant {
namespace {

// Valid stage_count clamped to [0, kMaxStages].
int stage_count_of(const PlantTypeConfig& type) {
  return std::clamp(type.stage_count, 0, static_cast<int>(kMaxStages));
}

// Death probability for the given age stage, clamped to [0, 1].
double death_rate_for_stage(const PlantTypeConfig& type, int stage) {
  const int stage_count = stage_count_of(type);
  if (stage_count <= 0) {
    return 0.0;
  }
  const int index = std::clamp(stage, 0, stage_count - 1);
  return std::clamp(type.death_rate_by_stage[static_cast<std::size_t>(index)],
                    0.0, 1.0);
}

// Species-default offspring rates used when seeding new individuals.
Fertility default_fertility(const PlantTypeConfig& type) {
  Fertility fertility;
  fertility.rate_by_stage = type.offspring_rate_by_stage;
  return fertility;
}

// Copy parent fertility and apply uniform +/- jitter per stage.
Fertility inherit_fertility(const Fertility& parent, int stage_count,
                            double jitter, std::mt19937& rng) {
  Fertility child = parent;
  if (jitter <= 0.0) {
    return child;
  }
  std::uniform_real_distribution<double> delta(-jitter, jitter);
  for (int i = 0; i < stage_count; ++i) {
    auto& rate = child.rate_by_stage[static_cast<std::size_t>(i)];
    rate = std::clamp(rate + delta(rng), 0.0, 1.0);
  }
  return child;
}

// Create a living plant entity at (x, y) with age 0 and the given fertility.
// Any eaten/cut residue on the cell is removed first.
entt::entity spawn_plant(entt::registry& registry, PlantKind kind,
                         const std::string& species_name,
                         const Visual& visual, int x, int y,
                         const Fertility& fertility) {
  clear_residue_at(registry, x, y);
  const entt::entity entity = registry.create();
  registry.emplace<Plant>(entity);
  registry.emplace<plant::PlantTypeIndex>(entity, plant::PlantTypeIndex{kind});
  registry.emplace<SpeciesName>(entity, SpeciesName{species_name});
  registry.emplace<Visual>(entity, visual);
  registry.emplace<Position>(entity, Position{x, y});
  registry.emplace<Age>(entity, Age{0});
  registry.emplace<Fertility>(entity, fertility);
  return entity;
}

// Increment Age.cycles for every living plant.
void age_plants(entt::registry& registry) {
  auto view = registry.view<Plant, Age>();
  for (auto entity : view) {
    ++view.get<Age>(entity).cycles;
  }
}

// Sample stage death rates, destroy plants that die this cycle and emit a
// Death event for each.
void kill_plants(entt::registry& registry, const RandomPlantConfig& config,
                 int cycles_per_time_unit, std::mt19937& rng,
                 events::EventQueue& event_queue) {
  std::vector<entt::entity> to_destroy;

  auto view = registry.view<Plant, plant::PlantTypeIndex, Age, Position>();
  for (auto entity : view) {
    const auto kind_index =
        static_cast<std::size_t>(view.get<plant::PlantTypeIndex>(entity).kind);
    if (kind_index >= config.plant_types.size()) {
      continue;
    }
    const auto& age = view.get<Age>(entity);
    const auto& pos = view.get<Position>(entity);
    const auto& type = config.plant_types[kind_index];
    const int stage = stage_at(age.cycles, type, cycles_per_time_unit);
    const double rate = death_rate_for_stage(type, stage);
    std::bernoulli_distribution dies(rate);
    if (dies(rng)) {
      event_queue.emit(events::Event::death(entity, type.name, pos.x, pos.y,
                                            age.cycles, stage));
      to_destroy.push_back(entity);
    }
  }

  for (auto entity : to_destroy) {
    registry.destroy(entity);
  }
}

// Sample fertility, place offspring on free cells, respect max_population.
// Emits a Birth event for each offspring.
void reproduce_plants(entt::registry& registry, const RandomPlantConfig& config,
                      int cycles_per_time_unit, const OccupancyPort& occupancy,
                      std::mt19937& rng, events::EventQueue& event_queue) {
  struct Pending {
    entt::entity parent;
    PlantKind kind;
    Fertility fertility;
  };

  auto free = occupancy.free_cells(registry);
  if (free.empty()) {
    return;
  }

  std::vector<Pending> pending;
  auto view = registry.view<Plant, plant::PlantTypeIndex, Age, Fertility>();
  for (auto entity : view) {
    const auto kind = view.get<plant::PlantTypeIndex>(entity).kind;
    const auto kind_index = static_cast<std::size_t>(kind);
    if (kind_index >= config.plant_types.size()) {
      continue;
    }
    const auto& fertility = view.get<Fertility>(entity);
    const auto& type = config.plant_types[kind_index];
    const int stage = stage_at(view.get<Age>(entity).cycles, type,
                               cycles_per_time_unit);
    const double rate = fertility.rate_for_stage(stage, stage_count_of(type));
    std::bernoulli_distribution reproduces(rate);
    if (reproduces(rng)) {
      pending.push_back(Pending{entity, kind, fertility});
    }
  }

  std::shuffle(pending.begin(), pending.end(), rng);
  if (pending.size() > free.size()) {
    pending.resize(free.size());
  }

  auto projected =
      plant::count_alive_by_kind<Plant>(registry, config.plant_types.size());

  for (const auto& parent : pending) {
    if (free.empty()) {
      break;
    }
    const auto kind_index = static_cast<std::size_t>(parent.kind);
    const auto& type = config.plant_types[kind_index];
    if (type.max_population > 0 &&
        projected[kind_index] >= type.max_population) {
      continue;
    }

    std::uniform_int_distribution<std::size_t> pick(0, free.size() - 1);
    const std::size_t index = pick(rng);
    const auto cell = free[index];
    free[index] = free.back();
    free.pop_back();

    const entt::entity child = spawn_plant(
        registry, parent.kind, type.name,
        resolve_visual(type.visual, default_shape(parent.kind), color_for_kind(parent.kind)),
        cell.first, cell.second,
        inherit_fertility(parent.fertility, stage_count_of(type),
                          config.offspring_inheritance_jitter, rng));
    ++projected[kind_index];
    event_queue.emit(events::Event::birth(child, parent.parent, type.name,
                                          cell.first, cell.second));
  }
}

// Top up each species to min_population by spawning on free cells.
// Emits a Birth event (without parent) for each seeded plant.
void seed_plants(entt::registry& registry, const RandomPlantConfig& config,
                 const OccupancyPort& occupancy, std::mt19937& rng,
                 events::EventQueue& event_queue) {
  auto free = occupancy.free_cells(registry);
  if (free.empty() || config.plant_types.empty()) {
    return;
  }

  const auto alive =
      plant::count_alive_by_kind<Plant>(registry, config.plant_types.size());
  std::vector<PlantKind> to_seed;
  for (std::size_t i = 0; i < config.plant_types.size(); ++i) {
    const auto& type = config.plant_types[i];
    int target = type.min_population;
    if (type.max_population > 0) {
      target = std::min(target, type.max_population);
    }
    const int missing = target - alive[i];
    for (int n = 0; n < missing; ++n) {
      to_seed.push_back(static_cast<PlantKind>(i));
    }
  }

  if (to_seed.size() > free.size()) {
    std::shuffle(to_seed.begin(), to_seed.end(), rng);
    to_seed.resize(free.size());
  }

  for (const PlantKind kind : to_seed) {
    std::uniform_int_distribution<std::size_t> pick(0, free.size() - 1);
    const std::size_t index = pick(rng);
    const auto cell = free[index];
    free[index] = free.back();
    free.pop_back();

    const auto& type = config.plant_types[static_cast<std::size_t>(kind)];
    const entt::entity entity =
        spawn_plant(registry, kind, type.name, resolve_visual(type.visual, default_shape(kind), color_for_kind(kind)), cell.first,
                    cell.second, default_fertility(type));
    event_queue.emit(events::Event::birth(entity, entt::null, type.name,
                                          cell.first, cell.second));
  }
}

}  // namespace

// Construct the process with species config and an occupancy port.
RandomPlantProcess::RandomPlantProcess(RandomPlantConfig config,
                                       OccupancyPort occupancy)
    : config_(std::move(config)), occupancy_(std::move(occupancy)) {
  if (!occupancy_.free_cells) {
    throw std::runtime_error("random_plant requires OccupancyPort::free_cells");
  }
}

// One lifecycle tick: age → stochastic death → reproduction → min seeding.
// Births and deaths are reported as events; the simulation tallies them.
void RandomPlantProcess::step(StepContext& ctx) {
  const int cycles_per_time_unit = ctx.simulation.cycles_per_time_unit;
  age_plants(ctx.registry);
  kill_plants(ctx.registry, config_, cycles_per_time_unit, ctx.rng, ctx.events);
  reproduce_plants(ctx.registry, config_, cycles_per_time_unit, occupancy_,
                   ctx.rng, ctx.events);
  seed_plants(ctx.registry, config_, occupancy_, ctx.rng, ctx.events);

  plant::publish_alive_by_name<Plant>(ctx.registry, config_.plant_types,
                                      ctx.tick);
}

}  // namespace random_plant
}  // namespace life_worlds
