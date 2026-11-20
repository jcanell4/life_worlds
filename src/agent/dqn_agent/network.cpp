#include "life_worlds/agent/dqn_agent/network.hpp"

#include <algorithm>
#include <iostream>

namespace life_worlds {
namespace dqn_agent {

DqnModelImpl::DqnModelImpl(int feature_size, int n_actions,
                           const std::vector<int>& hidden_sizes)
    : n_actions(n_actions) {
  torch::nn::Sequential seq;
  int in = feature_size;
  for (int hidden : hidden_sizes) {
    const int h = std::max(1, hidden);
    seq->push_back(torch::nn::Linear(in, h));
    seq->push_back(torch::nn::Functional(torch::relu));
    in = h;
  }
  seq->push_back(torch::nn::Linear(in, n_actions));
  net = register_module("net", seq);
}

torch::Tensor DqnModelImpl::forward(torch::Tensor x) {
  return net->forward(x);
}

DqnLearner::DqnLearner(const DqnAgentConfig& config, int feature_size)
    : n_actions_(kActionCount),
      gamma_(config.gamma),
      learning_rate_(config.learning_rate),
      feature_size_(std::max(1, feature_size)) {
  online_ = DqnModel(feature_size_, n_actions_, config.hidden_sizes);
  target_ = DqnModel(feature_size_, n_actions_, config.hidden_sizes);
  sync_target();
  optimizer_ = std::make_unique<torch::optim::Adam>(
      online_->parameters(), torch::optim::AdamOptions(learning_rate_));
}

int DqnLearner::best_action(
    const std::array<float, kMaxFeatureSize>& features) {
  torch::NoGradGuard no_grad;
  auto x = torch::from_blob(const_cast<float*>(features.data()),
                            {1, feature_size_}, torch::kFloat32)
               .clone();
  auto q = online_->forward(x);
  return static_cast<int>(q.argmax(/*dim=*/1).item<int64_t>());
}

double DqnLearner::train_step(const std::vector<Transition>& batch) {
  if (batch.empty()) {
    return 0.0;
  }
  const int b = static_cast<int>(batch.size());
  auto features = torch::empty({b, feature_size_}, torch::kFloat32);
  auto next_features = torch::empty({b, feature_size_}, torch::kFloat32);
  auto actions = torch::empty({b}, torch::kInt64);
  auto rewards = torch::empty({b}, torch::kFloat32);
  auto dones = torch::empty({b}, torch::kFloat32);

  auto f_acc = features.accessor<float, 2>();
  auto nf_acc = next_features.accessor<float, 2>();
  auto a_acc = actions.accessor<int64_t, 1>();
  auto r_acc = rewards.accessor<float, 1>();
  auto d_acc = dones.accessor<float, 1>();
  for (int i = 0; i < b; ++i) {
    const auto& t = batch[static_cast<std::size_t>(i)];
    for (int j = 0; j < feature_size_; ++j) {
      f_acc[i][j] = t.features[static_cast<std::size_t>(j)];
      nf_acc[i][j] = t.next_features[static_cast<std::size_t>(j)];
    }
    a_acc[i] = t.action;
    r_acc[i] = t.reward;
    d_acc[i] = t.done ? 1.0f : 0.0f;
  }

  torch::Tensor targets;
  {
    torch::NoGradGuard no_grad;
    // Double DQN: online picks the action, target evaluates it.
    auto next_q_online = online_->forward(next_features);
    auto next_actions = next_q_online.argmax(/*dim=*/1);
    auto next_q_target = target_->forward(next_features);
    auto next_values =
        next_q_target.gather(/*dim=*/1, next_actions.unsqueeze(1)).squeeze(1);
    targets = rewards + (1.0 - dones) * gamma_ * next_values;
  }

  auto q_all = online_->forward(features);
  auto q_sa = q_all.gather(/*dim=*/1, actions.unsqueeze(1)).squeeze(1);
  auto loss = torch::smooth_l1_loss(q_sa, targets);

  optimizer_->zero_grad();
  loss.backward();
  optimizer_->step();
  return loss.item<double>();
}

void DqnLearner::sync_target() {
  torch::NoGradGuard no_grad;
  auto online_params = online_->named_parameters(true);
  auto target_params = target_->named_parameters(true);
  for (auto& item : online_params) {
    target_params[item.key()].copy_(item.value());
  }
  auto online_buffers = online_->named_buffers(true);
  auto target_buffers = target_->named_buffers(true);
  for (auto& item : online_buffers) {
    target_buffers[item.key()].copy_(item.value());
  }
}

bool DqnLearner::save(const std::string& path) const {
  if (path.empty()) {
    return false;
  }
  try {
    torch::serialize::OutputArchive archive;
    online_->save(archive);
    archive.save_to(path);
    return true;
  } catch (const std::exception& ex) {
    std::cerr << "dqn_agent: save failed: " << ex.what() << '\n';
    return false;
  }
}

bool DqnLearner::load(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  try {
    torch::serialize::InputArchive archive;
    archive.load_from(path);
    online_->load(archive);
    sync_target();
    return true;
  } catch (const std::exception& ex) {
    std::cerr << "dqn_agent: load failed: " << ex.what() << '\n';
    return false;
  }
}

}  // namespace dqn_agent
}  // namespace life_worlds
