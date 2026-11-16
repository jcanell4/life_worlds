#include "life_worlds/app/grid_compose.hpp"

#include "life_worlds/agent/ql_agent/config.hpp"
#include "life_worlds/agent/ql_agent/process.hpp"
#include "life_worlds/environment/grid/adapters.hpp"
#include "life_worlds/environment/grid/environment.hpp"
#include "life_worlds/environment/grid/layers.hpp"
#include "life_worlds/plant/energy_storage_plant/config.hpp"
#include "life_worlds/plant/energy_storage_plant/process.hpp"
#include "life_worlds/plant/random_plant/config.hpp"
#include "life_worlds/plant/random_plant/process.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
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

Shape parse_shape(const std::string& name) {
  if (name == "circle") {
    return Shape::Circle;
  }
  if (name == "square") {
    return Shape::Square;
  }
  if (name == "triangle") {
    return Shape::Triangle;
  }
  if (name == "diamond") {
    return Shape::Diamond;
  }
  if (name == "hexagon") {
    return Shape::Hexagon;
  }
  throw std::runtime_error(
      "visual.shape must be circle, square, triangle, diamond or hexagon "
      "(got \"" + name + "\")");
}

// Color as "#RRGGBB" / "#RRGGBBAA" or as an array [r, g, b] / [r, g, b, a].
Rgba parse_color(const json& value) {
  Rgba color;
  if (value.is_string()) {
    const std::string text = value.get<std::string>();
    const bool shape_ok = (text.size() == 7 || text.size() == 9) &&
                          text[0] == '#' &&
                          std::all_of(text.begin() + 1, text.end(), [](char c) {
                            return std::isxdigit(static_cast<unsigned char>(c));
                          });
    if (!shape_ok) {
      throw std::runtime_error(
          "visual.color string must look like \"#RRGGBB\" or \"#RRGGBBAA\" "
          "(got \"" + text + "\")");
    }
    const auto byte_at = [&text](std::size_t pos) {
      return static_cast<std::uint8_t>(
          std::stoul(text.substr(pos, 2), nullptr, 16));
    };
    color.r = byte_at(1);
    color.g = byte_at(3);
    color.b = byte_at(5);
    if (text.size() == 9) {
      color.a = byte_at(7);
    }
    return color;
  }
  if (!value.is_array() || (value.size() != 3 && value.size() != 4)) {
    throw std::runtime_error(
        "visual.color must be \"#RRGGBB\" or an array [r, g, b] / [r, g, b, a]");
  }
  std::array<std::uint8_t, 4> channels{{0, 0, 0, 255}};
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (!value.at(i).is_number_integer()) {
      throw std::runtime_error("visual.color channels must be integers");
    }
    const int channel = value.at(i).get<int>();
    if (channel < 0 || channel > 255) {
      throw std::runtime_error("visual.color channels must be in [0, 255]");
    }
    channels[i] = static_cast<std::uint8_t>(channel);
  }
  return Rgba{channels[0], channels[1], channels[2], channels[3]};
}

// Optional "visual": { "shape": ..., "color": ... } block of a species/agent.
Visual parse_visual(const json& owner) {
  Visual visual;
  if (!owner.contains("visual")) {
    return visual;
  }
  const json& obj = owner.at("visual");
  if (!obj.is_object()) {
    throw std::runtime_error("visual must be an object");
  }
  if (obj.contains("shape")) {
    visual.shape = parse_shape(obj.at("shape").get<std::string>());
  }
  if (obj.contains("color")) {
    visual.color = parse_color(obj.at("color"));
  }
  return visual;
}

random_plant::PlantTypeConfig parse_plant_type(const json& obj) {
  random_plant::PlantTypeConfig type;
  type.name = obj.at("name").get<std::string>();
  type.visual = parse_visual(obj);
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
    if (config.plant_types.size() >= plant::kMaxPlantKinds) {
      throw std::runtime_error("too many plant_types");
    }
    config.plant_types.push_back(parse_plant_type(entry));
  }
  if (config.plant_types.empty()) {
    throw std::runtime_error("random_plant plant_types must not be empty");
  }
  return config;
}

energy_storage_plant::PlantTypeConfig parse_energy_plant_type(
    const json& obj) {
  energy_storage_plant::PlantTypeConfig type;
  type.name = obj.at("name").get<std::string>();
  const std::string where = " for species: " + type.name;
  type.initial_count = obj.value("initial_count", type.initial_count);
  type.initial_energy = obj.value("initial_energy", type.initial_energy);
  type.growing_rate = obj.value("growing_rate", type.growing_rate);
  type.maximum_energy_limit =
      obj.value("maximum_energy_limit", type.maximum_energy_limit);
  type.maximum_cell_occupancy =
      obj.value("maximum_cell_occupancy", type.maximum_cell_occupancy);
  type.split_fraction = obj.value("split_fraction", type.split_fraction);
  type.visual = parse_visual(obj);

  if (type.initial_count < 0) {
    throw std::runtime_error("initial_count must be >= 0" + where);
  }
  if (type.maximum_energy_limit <= 0.0) {
    throw std::runtime_error("maximum_energy_limit must be > 0" + where);
  }
  if (type.initial_energy < 0.0 ||
      type.initial_energy > type.maximum_energy_limit) {
    throw std::runtime_error(
        "initial_energy must be in [0, maximum_energy_limit]" + where);
  }
  if (type.growing_rate < 0.0) {
    throw std::runtime_error("growing_rate must be >= 0" + where);
  }
  if (type.maximum_cell_occupancy < 1) {
    throw std::runtime_error("maximum_cell_occupancy must be >= 1" + where);
  }
  if (type.split_fraction <= 0.0 || type.split_fraction >= 1.0) {
    throw std::runtime_error("split_fraction must be in (0, 1)" + where);
  }
  return type;
}

energy_storage_plant::EnergyStoragePlantConfig parse_energy_storage_plant(
    const json& entity) {
  energy_storage_plant::EnergyStoragePlantConfig config;
  if (!entity.contains("plant_types") || !entity.at("plant_types").is_array()) {
    throw std::runtime_error("energy_storage_plant requires plant_types array");
  }
  for (const auto& entry : entity.at("plant_types")) {
    if (config.plant_types.size() >= plant::kMaxPlantKinds) {
      throw std::runtime_error("too many energy_storage_plant plant_types");
    }
    config.plant_types.push_back(parse_energy_plant_type(entry));
  }
  if (config.plant_types.empty()) {
    throw std::runtime_error(
        "energy_storage_plant plant_types must not be empty");
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
  config.visual = parse_visual(entity);
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
    } else if (type == "energy_storage_plant") {
      loaded.entities.push_back(
          std::make_unique<energy_storage_plant::EnergyStoragePlantProcess>(
              parse_energy_storage_plant(entity), occupancy));
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
