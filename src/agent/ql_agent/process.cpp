#include "life_worlds/agent/ql_agent/process.hpp"

#include "life_worlds/agent/ql_agent/components.hpp"
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
namespace ql_agent {
namespace {

// Keep energy within [0, energy_max].
int clamp_energy(int value, int energy_max) {
  return std::clamp(value, 0, std::max(1, energy_max));
}

// Shaping reward based on remaining energy band (higher energy → less penalty).
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

// Epsilon-greedy in Train; greedy best_action in Eval.
int pick_action(const QTable& table, std::uint64_t state, Mode mode,
                double epsilon, std::mt19937& rng) {
  if (mode == Mode::Train) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    if (unit(rng) < epsilon) {
      std::uniform_int_distribution<int> action(0, kActionCount - 1);
      return action(rng);
    }
  }
  return table.best_action(state);
}

}  // namespace

// Construct the process with an injected world port (sensing / actuation).
QlAgentProcess::QlAgentProcess(QlAgentConfig config, AgentWorldPort world)
    : config_(std::move(config)), world_(std::move(world)) {
  if (!world_.default_vision_range || !world_.observe || !world_.apply_action ||
      !world_.reset_position || !world_.free_spawn_cells) {
    throw std::runtime_error("ql_agent requires a complete AgentWorldPort");
  }
}

// Publish cumulative eaten/cut counts into this tick's HUD/log metrics.
void QlAgentProcess::publish_harvest_stats(TickAccumulator& tick) const {
  for (const auto& [name, count] : eaten_by_name_) {
    tick.eaten_by_name.push_back(NamedCount{name, count});
  }
  for (const auto& [name, count] : cut_by_name_) {
    tick.cut_by_name.push_back(NamedCount{name, count});
  }
}

// Vision range: explicit config, otherwise the port default (e.g. grid size).
int QlAgentProcess::max_vision() const {
  if (config_.max_vision > 0) {
    return config_.max_vision;
  }
  return std::max(1, world_.default_vision_range());
}

// One-time setup: optionally load Q-table and spawn agent_count entities.
void QlAgentProcess::ensure_spawned(StepContext& ctx) {
  if (spawned_) {
    return;
  }

  const bool should_load =
      !config_.q_table_path.empty() &&
      (config_.mode == Mode::Eval || config_.reuse_q_table);
  if (should_load) {
    q_loaded_ = q_table_.load(config_.q_table_path);
    if (q_loaded_) {
      std::cout << "ql_agent: loaded Q-table (" << q_table_.size()
                << " states) from " << config_.q_table_path << '\n';
    } else {
      std::cerr << "ql_agent: could not load Q-table from "
                << config_.q_table_path << " (starting empty)\n";
    }
  } else if (config_.mode == Mode::Train) {
    std::cout << "ql_agent: starting with empty Q-table"
              << " (set reuse_q_table=true to continue training)\n";
  }

  auto free = world_.free_spawn_cells(ctx.registry);
  std::shuffle(free.begin(), free.end(), ctx.rng);

  const int to_spawn = std::max(0, config_.agent_count);
  const int n = std::min(to_spawn, static_cast<int>(free.size()));
  for (int i = 0; i < n; ++i) {
    const auto cell = free[static_cast<std::size_t>(i)];
    const entt::entity entity = ctx.registry.create();
    ctx.registry.emplace<QlAgent>(entity);
    ctx.registry.emplace<Position>(entity, Position{cell.first, cell.second});
    ctx.registry.emplace<Energy>(entity, Energy{config_.energy_max});
    ctx.registry.emplace<EpisodeCycles>(entity, EpisodeCycles{0});
    ctx.registry.emplace<Visual>(entity, config_.visual);
  }
  spawned_ = true;
}

// Per-agent observe → choose action → world apply → reward / Q update.
void QlAgentProcess::step(StepContext& ctx) {
  ensure_spawned(ctx);

  const int vision_cap = max_vision();
  int alive = 0;
  int agent_index = 0;

  std::vector<entt::entity> agents;
  {
    auto view = ctx.registry.view<QlAgent>();
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

  for (auto agent : agents) {
    if (!ctx.registry.valid(agent) || !ctx.registry.all_of<QlAgent>(agent)) {
      continue;
    }
    if (ctx.registry.all_of<Inactive>(agent)) {
      ++agent_index;
      continue;
    }

    auto& energy = ctx.registry.get<Energy>(agent);
    auto& episode = ctx.registry.get<EpisodeCycles>(agent);
    const auto& pos = ctx.registry.get<Position>(agent);
    const std::string agent_label = "ql" + std::to_string(agent_index);

    const Vision before =
        world_.observe(ctx.registry, pos.x, pos.y, vision_cap, agent);
    const std::uint64_t state =
        encode_state(before, energy_bucket(energy.value, config_.energy_max));
    const int action_index =
        pick_action(q_table_, state, config_.mode, config_.epsilon, ctx.rng);
    const Action action = static_cast<Action>(action_index);

    const grid::ActionOutcome outcome = world_.apply_action(
        ctx.registry, agent, action, eat_params, ctx.events);
    if (outcome.eaten && !outcome.species_name.empty()) {
      ++eaten_by_name_[outcome.species_name];
    }
    if (outcome.cut && !outcome.species_name.empty()) {
      ++cut_by_name_[outcome.species_name];
    }

    energy.value =
        clamp_energy(energy.value - config_.energy_drain, config_.energy_max);
    ++episode.value;

    const double band = static_cast<double>(
        energy_band_reward(energy.value, config_.energy_max));
    const double reward = outcome.food_reward + band;

    const auto& pos_after = ctx.registry.get<Position>(agent);
    const Vision after = world_.observe(ctx.registry, pos_after.x, pos_after.y,
                                        vision_cap, agent);
    const std::uint64_t next_state =
        encode_state(after, energy_bucket(energy.value, config_.energy_max));

    if (config_.mode == Mode::Train) {
      q_table_.update(state, action_index, reward, next_state, config_.alpha,
                      config_.gamma);
    }

    if (energy.value <= 0) {
      if (config_.mode == Mode::Train) {
        std::ostringstream detail;
        detail << agent_label << " reset episode_cycles=" << episode.value;
        ctx.events.emit(events::Event{events::EventKind::AgentReset, agent,
                                      entt::null, agent_label, pos_after.x,
                                      pos_after.y, episode.value, detail.str()});
        energy.value = config_.energy_max;
        episode.value = 0;
        if (ctx.registry.all_of<Inactive>(agent)) {
          ctx.registry.remove<Inactive>(agent);
        }
        world_.reset_position(ctx.registry, agent, ctx.rng);
        ++alive;
        ctx.tick.energy_by_agent.push_back(
            NamedCount{agent_label, ctx.registry.get<Energy>(agent).value});
      } else {
        agent_scores_[static_cast<std::size_t>(agent_index)] =
            AgentScore{agent_label, episode.value, 0, true};
        ctx.registry.emplace_or_replace<Inactive>(agent);
        ctx.tick.energy_by_agent.push_back(NamedCount{agent_label, 0});
        std::ostringstream detail;
        detail << agent_label << " finished score=" << episode.value;
        ctx.events.emit(events::Event{events::EventKind::AgentFinished, agent,
                                      entt::null, agent_label, pos_after.x,
                                      pos_after.y, episode.value, detail.str()});
      }
    } else {
      ++alive;
      if (config_.mode == Mode::Eval) {
        agent_scores_[static_cast<std::size_t>(agent_index)] =
            AgentScore{agent_label, episode.value, energy.value, false};
      }
      ctx.tick.energy_by_agent.push_back(NamedCount{agent_label, energy.value});
    }
    ++agent_index;
  }

  ctx.tick.alive_by_name.push_back(NamedCount{"ql_agent", alive});
  publish_harvest_stats(ctx.tick);
}

// End of run: save Q-table in train mode; in eval print every agent's score;
// print harvest totals.
void QlAgentProcess::shutdown() {
  if (config_.mode == Mode::Train && !config_.q_table_path.empty()) {
    if (q_table_.save(config_.q_table_path)) {
      std::cout << "ql_agent: saved Q-table (" << q_table_.size()
                << " states) to " << config_.q_table_path << '\n';
    } else {
      std::cerr << "ql_agent: failed to save Q-table to "
                << config_.q_table_path << '\n';
    }
  }
  if (config_.mode == Mode::Eval && !agent_scores_.empty()) {
    // Every agent: final score if it died, cycles so far (+ energy left) if it
    // was still alive when the run ended.
    std::cout << "ql_agent: eval scores (cycles survived)\n";
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
    std::cout << "ql_agent: harvest";
    for (const auto& [name, count] : eaten_by_name_) {
      std::cout << " eaten_" << name << "=" << count;
    }
    for (const auto& [name, count] : cut_by_name_) {
      std::cout << " cut_" << name << "=" << count;
    }
    std::cout << '\n';
  }
}

}  // namespace ql_agent
}  // namespace life_worlds
