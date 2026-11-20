#pragma once

#include "life_worlds/agent/dqn_agent/config.hpp"
#include "life_worlds/agent/features.hpp"
#include "life_worlds/agent/replay_buffer.hpp"

#include <torch/torch.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace life_worlds {
namespace dqn_agent {

using agent::kMaxFeatureSize;
using agent::Transition;

// MLP that maps features → n_actions Q-values.
struct DqnModelImpl : torch::nn::Module {
  DqnModelImpl(int feature_size, int n_actions,
               const std::vector<int>& hidden_sizes);

  torch::Tensor forward(torch::Tensor x);

  int n_actions = 0;
  torch::nn::Sequential net{nullptr};
};
TORCH_MODULE(DqnModel);

// Online + target networks, optimizer, and Double DQN update.
class DqnLearner {
 public:
  DqnLearner(const DqnAgentConfig& config, int feature_size);

  // Greedy action for a single feature vector (no grad).
  int best_action(const std::array<float, kMaxFeatureSize>& features);

  // One minibatch Double DQN update from `batch`. Returns the scalar loss.
  double train_step(const std::vector<Transition>& batch);

  void sync_target();

  bool save(const std::string& path) const;
  bool load(const std::string& path);

 private:
  int n_actions_ = kActionCount;
  double gamma_ = 0.9;
  double learning_rate_ = 1e-3;
  int feature_size_ = agent::feature_size(4);

  DqnModel online_{nullptr};
  DqnModel target_{nullptr};
  std::unique_ptr<torch::optim::Adam> optimizer_;
};

}  // namespace dqn_agent
}  // namespace life_worlds
