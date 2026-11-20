#pragma once

#include "life_worlds/agent/c51_agent/config.hpp"
#include "life_worlds/agent/c51_agent/network.hpp"
#include "life_worlds/agent/replay_buffer.hpp"
#include "life_worlds/core/components.hpp"
#include "life_worlds/entity/entity.hpp"
#include "life_worlds/ports/agent_world_port.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace life_worlds {
namespace c51_agent {

// Spawns C51 agents, chooses actions, learns from environment outcomes.
class C51AgentProcess final : public Entity {
 public:
  C51AgentProcess(C51AgentConfig config, AgentWorldPort world);

  void step(StepContext& ctx) override;
  void shutdown() override;
  bool is_training() const override { return config_.mode == Mode::Train; }

  const C51AgentConfig& config() const { return config_; }

  struct AgentScore {
    std::string label;
    int cycles = 0;
    int energy = 0;
    bool finished = false;
  };
  const std::vector<AgentScore>& agent_scores() const { return agent_scores_; }

 private:
  void ensure_spawned(StepContext& ctx);
  Visual agent_visual() const;
  int max_vision() const;
  void publish_harvest_stats(TickAccumulator& tick) const;
  void maybe_learn(std::mt19937& rng);

  C51AgentConfig config_;
  AgentWorldPort world_;
  C51Learner learner_;
  agent::ReplayBuffer replay_;
  bool spawned_ = false;
  bool model_loaded_ = false;
  int train_steps_ = 0;
  int updates_ = 0;
  double last_loss_ = 0.0;
  std::vector<AgentScore> agent_scores_;
  std::unordered_map<std::string, int> eaten_by_name_;
  std::unordered_map<std::string, int> cut_by_name_;
};

}  // namespace c51_agent
}  // namespace life_worlds
