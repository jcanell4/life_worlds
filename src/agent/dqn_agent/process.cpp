#include "life_worlds/agent/dqn_agent/process.hpp"

#include "life_worlds/agent/components.hpp"
#include "life_worlds/agent/dqn_agent/components.hpp"
#include "life_worlds/agent/dqn_agent/visual.hpp"
#include "life_worlds/agent/features.hpp"
#include "life_worlds/agent/observation.hpp"
#include "life_worlds/core/components.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace life_worlds {
namespace dqn_agent {
namespace {

using agent::encode_features;
using agent::kMaxFeatureSize;
using agent::Transition;

int clamp_energy(int value, int energy_max) {
  return std::clamp(value, 0, std::max(1, energy_max));
}

int units(const Energy& energy) { return static_cast<int>(energy.value); }

int energy_band_reward(int energy, int energy_max) {
  if (energy_max <= 0) {
    return 0;
  }
  const double ratio =
      static_cast<double>(energy) / static_cast<double>(energy_max);
  if (ratio > 0.75) {
    return -1;
  }
  if (ratio > 0.50) {
    return -2;
  }
  if (ratio > 0.25) {
    return -3;
  }
  return -5;
}

int pick_action(DqnLearner& learner,
                const std::array<float, agent::kMaxFeatureSize>& features, Mode mode,
                double epsilon, std::mt19937& rng) {
  if (mode == Mode::Train) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    if (unit(rng) < epsilon) {
      std::uniform_int_distribution<int> action(0, kActionCount - 1);
      return action(rng);
    }
  }
  return learner.best_action(features);
}

}  // namespace

DqnAgentProcess::DqnAgentProcess(DqnAgentConfig config, AgentWorldPort world)
    : config_(std::move(config)),
      world_(std::move(world)),
      learner_(config_, agent::feature_size(config_.vision_rays)),
      replay_(config_.replay_capacity) {
  if (!world_.default_vision_range || !world_.observe || !world_.apply_action ||
      !world_.reset_position || !world_.free_spawn_cells) {
    throw std::runtime_error("dqn_agent requires a complete AgentWorldPort");
  }
  if (config_.vision_rays != 4 && config_.vision_rays != 8) {
    throw std::runtime_error("dqn_agent vision_rays must be 4 or 8");
  }
}

void DqnAgentProcess::publish_harvest_stats(TickAccumulator& tick) const {
  for (const auto& [name, count] : eaten_by_name_) {
    tick.eaten_by_name.push_back(NamedCount{name, count});
  }
  for (const auto& [name, count] : cut_by_name_) {
    tick.cut_by_name.push_back(NamedCount{name, count});
  }
}

int DqnAgentProcess::max_vision() const {
  if (config_.max_vision > 0) {
    return config_.max_vision;
  }
  return std::max(1, world_.default_vision_range());
}

Visual DqnAgentProcess::agent_visual() const {
  return resolve_visual(config_.visual, default_shape(), agent_color());
}

void DqnAgentProcess::ensure_spawned(StepContext& ctx) {
  if (spawned_) {
    return;
  }

  const bool should_load =
      !config_.model_path.empty() &&
      (config_.mode == Mode::Eval || config_.reuse_model);
  if (should_load) {
    model_loaded_ = learner_.load(config_.model_path);
    if (model_loaded_) {
      std::cout << "dqn_agent: loaded model from " << config_.model_path
                << '\n';
    } else {
      std::cerr << "dqn_agent: could not load model from "
                << config_.model_path << " (starting random)\n";
    }
  } else if (config_.mode == Mode::Train) {
    std::cout << "dqn_agent: starting with random weights"
              << " (set reuse_model=true to continue training)\n";
  }

  auto free = world_.free_spawn_cells(ctx.registry);
  std::shuffle(free.begin(), free.end(), ctx.rng);

  const int to_spawn = std::max(0, config_.agent_count);
  const int n = std::min(to_spawn, static_cast<int>(free.size()));
  const double energy_max = static_cast<double>(config_.energy_max);
  for (int i = 0; i < n; ++i) {
    const auto cell = free[static_cast<std::size_t>(i)];
    const entt::entity entity = ctx.registry.create();
    ctx.registry.emplace<agent::Agent>(entity);
    ctx.registry.emplace<DqnAgent>(entity);
    ctx.registry.emplace<Position>(entity, Position{cell.first, cell.second});
    ctx.registry.emplace<Energy>(entity, Energy{energy_max, energy_max});
    ctx.registry.emplace<agent::EpisodeCycles>(entity, agent::EpisodeCycles{0});
    ctx.registry.emplace<Visual>(entity, agent_visual());
  }
  spawned_ = true;
}

void DqnAgentProcess::maybe_learn(std::mt19937& rng) {
  if (config_.mode != Mode::Train) {
    return;
  }
  ++train_steps_;
  if (replay_.size() < config_.learn_start) {
    return;
  }
  if (config_.train_every > 0 && (train_steps_ % config_.train_every) != 0) {
    return;
  }
  std::vector<Transition> batch;
  if (!replay_.sample(config_.batch_size, batch, rng)) {
    return;
  }
  last_loss_ = learner_.train_step(batch);
  ++updates_;
  if (config_.target_update_every > 0 &&
      (updates_ % config_.target_update_every) == 0) {
    learner_.sync_target();
  }
}

void DqnAgentProcess::step(StepContext& ctx) {
  ensure_spawned(ctx);

  const int vision_cap = max_vision();
  int alive = 0;
  int agent_index = 0;

  std::vector<entt::entity> agents;
  {
    auto view = ctx.registry.view<DqnAgent>();
    agents.assign(view.begin(), view.end());
  }
  if (config_.mode == Mode::Eval && agent_scores_.size() < agents.size()) {
    agent_scores_.resize(agents.size());
  }

  const grid::EatParams eat_params{
      config_.herb_energy_delta,
      config_.shrub_energy_delta,
      config_.ingestion_factor[0],
      config_.ingestion_factor[1],
      config_.energy_max,
  };

  for (auto agent_entity : agents) {
    if (!ctx.registry.valid(agent_entity) ||
        !ctx.registry.all_of<DqnAgent>(agent_entity)) {
      continue;
    }
    if (ctx.registry.all_of<agent::Inactive>(agent_entity)) {
      ++agent_index;
      continue;
    }

    auto& energy = ctx.registry.get<Energy>(agent_entity);
    auto& episode = ctx.registry.get<agent::EpisodeCycles>(agent_entity);
    const auto& pos = ctx.registry.get<Position>(agent_entity);
    const std::string agent_label = "dqn" + std::to_string(agent_index);

    const agent::Vision before =
        world_.observe(ctx.registry, pos.x, pos.y, vision_cap, agent_entity,
                       config_.vision_rays);
    const auto features =
        encode_features(before, units(energy), config_.energy_max);
    const int action_index =
        pick_action(learner_, features, config_.mode, config_.epsilon, ctx.rng);
    const Action action = static_cast<Action>(action_index);

    const grid::ActionOutcome outcome = world_.apply_action(
        ctx.registry, agent_entity, action, eat_params, ctx.events);
    if (outcome.eaten && !outcome.species_name.empty()) {
      ++eaten_by_name_[outcome.species_name];
    }
    if (outcome.cut && !outcome.species_name.empty()) {
      ++cut_by_name_[outcome.species_name];
    }

    bool done = false;
    double reward = 0.0;
    if (outcome.fell) {
      energy.value = 0.0;
      reward = config_.precipice_reward;
      done = true;
    } else {
      energy.value = clamp_energy(units(energy) - config_.energy_drain,
                                  config_.energy_max);
      const double band = static_cast<double>(
          energy_band_reward(units(energy), config_.energy_max));
      reward = outcome.food_reward + band;
      done = units(energy) <= 0;
    }
    ++episode.value;

    const auto& pos_after = ctx.registry.get<Position>(agent_entity);
    const agent::Vision after = world_.observe(
        ctx.registry, pos_after.x, pos_after.y, vision_cap, agent_entity,
        config_.vision_rays);
    const auto next_features =
        encode_features(after, units(energy), config_.energy_max);

    if (config_.mode == Mode::Train) {
      Transition transition;
      transition.features = features;
      transition.action = action_index;
      transition.reward = static_cast<float>(reward);
      transition.next_features = next_features;
      transition.done = done;
      replay_.push(transition);
    }

    if (done) {
      if (config_.mode == Mode::Train) {
        std::ostringstream detail;
        detail << agent_label << " reset episode_cycles=" << episode.value;
        if (outcome.fell) {
          detail << " fell";
        }
        ctx.events.emit(events::Event{events::EventKind::AgentReset,
                                      agent_entity, entt::null, agent_label,
                                      pos_after.x, pos_after.y, episode.value,
                                      detail.str()});
        energy.value = config_.energy_max;
        episode.value = 0;
        if (ctx.registry.all_of<agent::Inactive>(agent_entity)) {
          ctx.registry.remove<agent::Inactive>(agent_entity);
          ctx.registry.emplace_or_replace<Visual>(agent_entity, agent_visual());
        }
        world_.reset_position(ctx.registry, agent_entity, ctx.rng);
        ++alive;
        ctx.tick.energy_by_agent.push_back(
            NamedCount{agent_label, units(energy)});
      } else {
        agent_scores_[static_cast<std::size_t>(agent_index)] =
            AgentScore{agent_label, episode.value, 0, true};
        ctx.registry.emplace_or_replace<agent::Inactive>(agent_entity);
        ctx.registry.remove<Visual>(agent_entity);
        ctx.tick.energy_by_agent.push_back(NamedCount{agent_label, 0});
        std::ostringstream detail;
        detail << agent_label << " finished score=" << episode.value;
        if (outcome.fell) {
          detail << " fell";
        }
        ctx.events.emit(events::Event{events::EventKind::AgentFinished,
                                      agent_entity, entt::null, agent_label,
                                      pos_after.x, pos_after.y, episode.value,
                                      detail.str()});
      }
    } else {
      ++alive;
      if (config_.mode == Mode::Eval) {
        agent_scores_[static_cast<std::size_t>(agent_index)] =
            AgentScore{agent_label, episode.value, units(energy), false};
      }
      ctx.tick.energy_by_agent.push_back(
          NamedCount{agent_label, units(energy)});
    }
    ++agent_index;
  }

  maybe_learn(ctx.rng);

  ctx.tick.alive_by_name.push_back(NamedCount{"dqn_agent", alive});
  publish_harvest_stats(ctx.tick);
}

void DqnAgentProcess::shutdown() {
  if (config_.mode == Mode::Train && !config_.model_path.empty()) {
    if (learner_.save(config_.model_path)) {
      std::cout << "dqn_agent: saved model to " << config_.model_path
                << " (updates=" << updates_ << " last_loss=" << last_loss_
                << " replay=" << replay_.size() << ")\n";
    } else {
      std::cerr << "dqn_agent: failed to save model to " << config_.model_path
                << '\n';
    }
  }
  if (config_.mode == Mode::Eval && !agent_scores_.empty()) {
    std::cout << "dqn_agent: eval scores (cycles survived)\n";
    const int energy_max = std::max(1, config_.energy_max);
    for (const AgentScore& score : agent_scores_) {
      std::cout << "  " << score.label << ": " << score.cycles;
      if (score.finished) {
        std::cout << " (died)\n";
      } else {
        const int percent = score.energy * 100 / energy_max;
        std::cout << " (alive - " << percent << "%, energy " << score.energy
                  << "/" << energy_max << ")\n";
      }
    }
  }
  if (!eaten_by_name_.empty() || !cut_by_name_.empty()) {
    std::cout << "dqn_agent: harvest";
    for (const auto& [name, count] : eaten_by_name_) {
      std::cout << " eaten_" << name << "=" << count;
    }
    for (const auto& [name, count] : cut_by_name_) {
      std::cout << " cut_" << name << "=" << count;
    }
    std::cout << '\n';
  }
}

}  // namespace dqn_agent
}  // namespace life_worlds
