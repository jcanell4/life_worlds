#include "life_worlds/app/grid_compose.hpp"

#include "life_worlds/agent/ql_agent/config.hpp"
#include "life_worlds/agent/ql_agent/process.hpp"
#include "life_worlds/environment/grid/adapters.hpp"
#include "life_worlds/environment/grid/environment.hpp"
#include "life_worlds/environment/grid/layers.hpp"
#include "life_worlds/plant/random_plant/config.hpp"
#include "life_worlds/plant/random_plant/process.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace life_worlds {
namespace app {
namespace {

using json = nlohmann::json;

std::array<double, random_plant::kMaxStages> read_double_stages(
    const json& arr, int stage_count) {
  std::array<double, random_plant::kMaxStages> out{};
  if (!arr.is_array()) {
    throw std::runtime_error("stage rate table must be an array");
  }
  const int n = std::min(stage_count, static_cast<int>(arr.size()));
  for (int i = 0; i < n; ++i) {
    out[static_cast<std::size_t>(i)] = arr.at(i).get<double>();
  }
  return out;
}

std::array<int, random_plant::kMaxStages> read_int_stages(const json& arr,
                                                          int stage_count) {
  std::array<int, random_plant::kMaxStages> out{};
  if (arr.is_null()) {
    return out;
  }
  if (!arr.is_array()) {
    throw std::runtime_error("time_units_by_stage must be an array");
  }
  const int n = std::min(stage_count, static_cast<int>(arr.size()));
  for (int i = 0; i < n; ++i) {
    out[static_cast<std::size_t>(i)] = arr.at(i).get<int>();
  }
  return out;
}

random_plant::PlantTypeConfig parse_plant_type(const json& obj) {
  random_plant::PlantTypeConfig type;
  type.name = obj.at("name").get<std::string>();
  type.min_population = obj.value("min_population", 0);
  type.max_population = obj.value("max_population", 0);
  if (type.max_population < 0) {
    throw std::runtime_error("max_population must be >= 0 for species: " +
                             type.name);
  }
  if (type.max_population > 0 && type.min_population > type.max_population) {
    throw std::runtime_error(
        "min_population cannot exceed max_population for species: " +
        type.name);
  }
  type.stage_count = obj.at("stage_count").get<int>();
  if (type.stage_count < 0 ||
      type.stage_count > static_cast<int>(random_plant::kMaxStages)) {
    throw std::runtime_error("stage_count out of range for species: " +
                             type.name);
  }
  type.death_rate_by_stage =
      read_double_stages(obj.at("death_rate_by_stage"), type.stage_count);
  type.offspring_rate_by_stage =
      read_double_stages(obj.at("offspring_rate_by_stage"), type.stage_count);
  if (obj.contains("time_units_by_stage")) {
    type.time_units_by_stage =
        read_int_stages(obj.at("time_units_by_stage"), type.stage_count);
  }
  return type;
}

random_plant::RandomPlantConfig parse_random_plant(const json& entity) {
  random_plant::RandomPlantConfig config;
  config.offspring_inheritance_jitter =
      entity.value("offspring_inheritance_jitter", 0.01);
  if (!entity.contains("plant_types") || !entity.at("plant_types").is_array()) {
    throw std::runtime_error("random_plant requires plant_types array");
  }
  for (const auto& entry : entity.at("plant_types")) {
    if (config.plant_types.size() >= random_plant::kMaxPlantKinds) {
      throw std::runtime_error("too many plant_types");
    }
    config.plant_types.push_back(parse_plant_type(entry));
  }
  if (config.plant_types.empty()) {
    throw std::runtime_error("random_plant plant_types must not be empty");
  }
  return config;
}

ql_agent::QlAgentConfig parse_ql_agent(const json& entity) {
  ql_agent::QlAgentConfig config;
  const std::string mode = entity.value("mode", std::string("train"));
  if (mode == "train") {
    config.mode = ql_agent::Mode::Train;
  } else if (mode == "eval") {
    config.mode = ql_agent::Mode::Eval;
  } else {
    throw std::runtime_error("ql_agent mode must be \"train\" or \"eval\"");
  }
  config.agent_count = entity.value("agent_count", config.agent_count);
  config.energy_max = entity.value("energy_max", config.energy_max);
  config.energy_drain = entity.value("energy_drain", config.energy_drain);
  config.herb_energy_delta =
      entity.value("herb_energy_delta", config.herb_energy_delta);
  config.shrub_energy_delta =
      entity.value("shrub_energy_delta", config.shrub_energy_delta);
  if (entity.contains("ingestion_factor")) {
    const json& factors = entity.at("ingestion_factor");
    if (!factors.is_array()) {
      throw std::runtime_error("ql_agent ingestion_factor must be an array");
    }
    for (std::size_t i = 0;
         i < config.ingestion_factor.size() && i < factors.size(); ++i) {
      config.ingestion_factor[i] = factors.at(i).get<double>();
    }
  }
  config.alpha = entity.value("alpha", config.alpha);
  config.gamma = entity.value("gamma", config.gamma);
  config.epsilon = entity.value("epsilon", config.epsilon);
  config.max_vision = entity.value("max_vision", config.max_vision);
  config.q_table_path = entity.value("q_table_path", config.q_table_path);
  config.reuse_q_table = entity.value("reuse_q_table", config.reuse_q_table);
  if (config.agent_count < 0) {
    throw std::runtime_error("ql_agent agent_count must be >= 0");
  }
  return config;
}

}  // namespace

void compose_grid_app(const json& root, LoadedConfig& loaded) {
  if (!root.contains("environment")) {
    throw std::runtime_error("config requires an environment object");
  }
  const json& env_json = root.at("environment");
  const std::string env_type = env_json.value("type", std::string("grid"));
  if (env_type != "grid") {
    throw std::runtime_error("unsupported environment type: " + env_type);
  }

  grid::GridConfig grid_config;
  grid_config.grid_size = env_json.value("grid_size", grid_config.grid_size);
  if (grid_config.grid_size <= 0) {
    throw std::runtime_error("environment.grid_size must be > 0");
  }

  auto grid_env = std::make_unique<grid::GridEnvironment>(grid_config);
  grid::register_default_render_layers(*grid_env);

  OccupancyPort occupancy = grid::make_occupancy_port(*grid_env);
  AgentWorldPort agent_world = grid::make_agent_world_port(*grid_env);

  if (!root.contains("entities") || !root.at("entities").is_array()) {
    throw std::runtime_error("config requires a non-empty entities array");
  }
  for (const auto& entity : root.at("entities")) {
    const std::string type = entity.at("type").get<std::string>();
    if (type == "random_plant") {
      loaded.entities.push_back(
          std::make_unique<random_plant::RandomPlantProcess>(
              parse_random_plant(entity), occupancy));
    } else if (type == "ql_agent") {
      loaded.entities.push_back(std::make_unique<ql_agent::QlAgentProcess>(
          parse_ql_agent(entity), agent_world));
    } else {
      throw std::runtime_error("unknown entity type: " + type);
    }
  }
  if (loaded.entities.empty()) {
    throw std::runtime_error("entities array must not be empty");
  }

  loaded.environment = std::move(grid_env);
}

}  // namespace app
}  // namespace life_worlds
