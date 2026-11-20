#pragma once

#include "life_worlds/agent/c51_agent/config.hpp"
#include "life_worlds/agent/features.hpp"
#include "life_worlds/agent/replay_buffer.hpp"

#include <torch/torch.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace life_worlds {
namespace c51_agent {

using agent::kMaxFeatureSize;
using agent::Transition;

// MLP that maps features → (n_actions × n_atoms) logits.
struct C51ModelImpl : torch::nn::Module {
  C51ModelImpl(int feature_size, int n_actions, int n_atoms,
               const std::vector<int>& hidden_sizes);

  torch::Tensor forward(torch::Tensor x);

  int n_actions = 0;
  int n_atoms = 0;
  torch::nn::Sequential net{nullptr};
};
TORCH_MODULE(C51Model);

// Online + target networks, optimizer, and C51 projection update.
class C51Learner {
 public:
  C51Learner(const C51AgentConfig& config, int feature_size);

  // Greedy action for a single feature vector (no grad).
  int best_action(const std::array<float, kMaxFeatureSize>& features);

  // One minibatch C51 update from `batch`. Returns the scalar loss.
  double train_step(const std::vector<Transition>& batch);

  void sync_target();

  bool save(const std::string& path) const;
  bool load(const std::string& path);

  int n_atoms() const { return n_atoms_; }
  double v_min() const { return v_min_; }
  double v_max() const { return v_max_; }

 private:
  int n_actions_ = kActionCount;
  int n_atoms_ = 51;
  double v_min_ = -20.0;
  double v_max_ = 20.0;
  double gamma_ = 0.9;
  double learning_rate_ = 1e-3;
  int feature_size_ = agent::feature_size(4);

  torch::Tensor support_;  // [n_atoms] atom values z_i
  C51Model online_{nullptr};
  C51Model target_{nullptr};
  std::unique_ptr<torch::optim::Adam> optimizer_;
};

}  // namespace c51_agent
}  // namespace life_worlds
