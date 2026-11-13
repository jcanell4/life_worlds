#pragma once

#include "life_worlds/agent/ql_agent/config.hpp"
#include "life_worlds/agent/ql_agent/q_table.hpp"
#include "life_worlds/entity/entity.hpp"
#include "life_worlds/ports/agent_world_port.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace life_worlds {
namespace ql_agent {

// Spawns QL agents, chooses actions, learns from environment outcomes.
class QlAgentProcess final : public Entity {
 public:
  QlAgentProcess(QlAgentConfig config, AgentWorldPort world);

  void step(StepContext& ctx) override;
  void shutdown() override;

  const QlAgentConfig& config() const { return config_; }
  const std::vector<int>& episode_scores() const { return episode_scores_; }

 private:
  void ensure_spawned(StepContext& ctx);
  int max_vision() const;
  void publish_harvest_stats(TickAccumulator& tick) const;

  QlAgentConfig config_;
  AgentWorldPort world_;
  QTable q_table_;
  bool spawned_ = false;
  bool q_loaded_ = false;
  std::vector<int> episode_scores_;
  std::unordered_map<std::string, int> eaten_by_name_;
  std::unordered_map<std::string, int> cut_by_name_;
};

}  // namespace ql_agent
}  // namespace life_worlds
