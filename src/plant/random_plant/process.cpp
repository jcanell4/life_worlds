#include "life_worlds/plant/random_plant/process.hpp"

#include "life_worlds/core/components.hpp"
#include "life_worlds/plant/random_plant/components.hpp"

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

struct BirthRecord {
  entt::entity entity{};
  PlantKind kind = 0;
  int x = 0;
  int y = 0;
  entt::entity parent = entt::null;
};

struct DeathRecord {
  entt::entity entity{};
  PlantKind kind = 0;
  int age_cycles = 0;
  int age_stage = 0;
  int x = 0;
  int y = 0;
};

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

// Remove any eaten/cut residue markers on cell (x, y) before a live spawn.
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

// Create a living plant entity at (x, y) with age 0 and the given fertility.
entt::entity spawn_plant(entt::registry& registry, PlantKind kind,
                         const std::string& species_name, int x, int y,
                         const Fertility& fertility) {
  clear_residue_at(registry, x, y);
  const entt::entity entity = registry.create();
  registry.emplace<Plant>(entity);
  registry.emplace<PlantType>(entity, PlantType{kind});
  registry.emplace<SpeciesName>(entity, SpeciesName{species_name});
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

// Living population count indexed by PlantKind.
std::vector<int> count_alive_by_kind(const entt::registry& registry,
                                     std::size_t kind_count) {
  std::vector<int> alive(kind_count, 0);
  auto view = registry.view<const Plant, const PlantType>();
  for (auto entity : view) {
    const auto kind = view.get<const PlantType>(entity).kind;
    if (static_cast<std::size_t>(kind) < kind_count) {
      ++alive[static_cast<std::size_t>(kind)];
    }
  }
  return alive;
}

// Sample stage death rates and destroy plants that die this cycle.
std::vector<DeathRecord> kill_plants(entt::registry& registry,
                                     const RandomPlantConfig& config,
                                     int cycles_per_time_unit,
                                     std::mt19937& rng) {
  std::vector<DeathRecord> events;
  std::vector<entt::entity> to_destroy;

  auto view = registry.view<Plant, PlantType, Age, Position>();
  for (auto entity : view) {
    const auto& type_comp = view.get<PlantType>(entity);
    const auto kind_index = static_cast<std::size_t>(type_comp.kind);
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
      events.push_back(
          DeathRecord{entity, type_comp.kind, age.cycles, stage, pos.x, pos.y});
      to_destroy.push_back(entity);
    }
  }

  for (auto entity : to_destroy) {
    registry.destroy(entity);
  }
  return events;
}

// Sample fertility, place offspring on free cells, respect max_population.
std::vector<BirthRecord> reproduce_plants(entt::registry& registry,
                                          const RandomPlantConfig& config,
                                          int cycles_per_time_unit,
                                          const OccupancyPort& occupancy,
                                          std::mt19937& rng) {
  struct Pending {
    entt::entity parent;
    PlantKind kind;
    Fertility fertility;
  };

  std::vector<BirthRecord> events;
  auto free = occupancy.free_cells(registry);
  if (free.empty()) {
    return events;
  }

  std::vector<Pending> pending;
  auto view = registry.view<Plant, PlantType, Age, Fertility>();
  for (auto entity : view) {
    const auto kind = view.get<PlantType>(entity).kind;
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

  auto projected = count_alive_by_kind(registry, config.plant_types.size());
  events.reserve(pending.size());

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
        registry, parent.kind, type.name, cell.first, cell.second,
        inherit_fertility(parent.fertility, stage_count_of(type),
                          config.offspring_inheritance_jitter, rng));
    ++projected[kind_index];
    events.push_back(
        BirthRecord{child, parent.kind, cell.first, cell.second, parent.parent});
  }
  return events;
}

// Top up each species to min_population by spawning on free cells.
std::vector<BirthRecord> seed_plants(entt::registry& registry,
                                     const RandomPlantConfig& config,
                                     const OccupancyPort& occupancy,
                                     std::mt19937& rng) {
  std::vector<BirthRecord> events;
  auto free = occupancy.free_cells(registry);
  if (free.empty() || config.plant_types.empty()) {
    return events;
  }

  const auto alive = count_alive_by_kind(registry, config.plant_types.size());
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
  events.reserve(to_seed.size());

  for (const PlantKind kind : to_seed) {
    std::uniform_int_distribution<std::size_t> pick(0, free.size() - 1);
    const std::size_t index = pick(rng);
    const auto cell = free[index];
    free[index] = free.back();
    free.pop_back();

    const auto& type = config.plant_types[static_cast<std::size_t>(kind)];
    const entt::entity entity =
        spawn_plant(registry, kind, type.name, cell.first, cell.second,
                    default_fertility(type));
    events.push_back(BirthRecord{entity, kind, cell.first, cell.second});
  }
  return events;
}

// Write plant birth/death/alive metrics and emit typed events.
void fill_tick(TickAccumulator& tick, events::EventQueue& event_queue,
               const RandomPlantConfig& config,
               const std::vector<BirthRecord>& births,
               const std::vector<DeathRecord>& deaths,
               const std::vector<int>& alive) {
  const std::size_t n = config.plant_types.size();
  std::vector<int> births_by(n, 0);
  std::vector<int> deaths_by(n, 0);

  for (const auto& event : births) {
    const auto i = static_cast<std::size_t>(event.kind);
    if (i < n) {
      ++births_by[i];
    }
  }
  for (const auto& event : deaths) {
    const auto i = static_cast<std::size_t>(event.kind);
    if (i < n) {
      ++deaths_by[i];
    }
  }

  tick.births += static_cast<int>(births.size());
  tick.deaths += static_cast<int>(deaths.size());

  for (std::size_t i = 0; i < n; ++i) {
    const std::string& name = config.plant_types[i].name;
    tick.alive_by_name.push_back(NamedCount{name, alive[i]});
    if (births_by[i] != 0) {
      tick.births_by_name.push_back(NamedCount{name, births_by[i]});
    }
    if (deaths_by[i] != 0) {
      tick.deaths_by_name.push_back(NamedCount{name, deaths_by[i]});
    }
  }

  for (const auto& event : births) {
    const auto i = static_cast<std::size_t>(event.kind);
    const std::string name = i < n ? config.plant_types[i].name : "?";
    event_queue.emit(events::Event{events::EventKind::Birth, event.entity,
                                   event.parent, name, event.x, event.y, 0, {}});
  }
  for (const auto& event : deaths) {
    const auto i = static_cast<std::size_t>(event.kind);
    const std::string name = i < n ? config.plant_types[i].name : "?";
    event_queue.emit(events::Event{events::EventKind::Death, event.entity,
                                   entt::null, name, event.x, event.y,
                                   event.age_cycles,
                                   std::to_string(event.age_stage)});
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
void RandomPlantProcess::step(StepContext& ctx) {
  age_plants(ctx.registry);
  const auto deaths = kill_plants(ctx.registry, config_,
                                  ctx.simulation.cycles_per_time_unit, ctx.rng);
  auto births = reproduce_plants(ctx.registry, config_,
                                 ctx.simulation.cycles_per_time_unit,
                                 occupancy_, ctx.rng);
  const auto seeded =
      seed_plants(ctx.registry, config_, occupancy_, ctx.rng);
  births.insert(births.end(), seeded.begin(), seeded.end());

  const auto alive =
      count_alive_by_kind(ctx.registry, config_.plant_types.size());
  fill_tick(ctx.tick, ctx.events, config_, births, deaths, alive);
}

}  // namespace random_plant
}  // namespace life_worlds
