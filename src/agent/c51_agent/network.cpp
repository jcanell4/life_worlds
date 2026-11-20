#include "life_worlds/agent/c51_agent/network.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace life_worlds {
namespace c51_agent {

C51ModelImpl::C51ModelImpl(int feature_size, int n_actions, int n_atoms,
                           const std::vector<int>& hidden_sizes)
    : n_actions(n_actions), n_atoms(n_atoms) {
  torch::nn::Sequential seq;
  int in = feature_size;
  for (int hidden : hidden_sizes) {
    const int h = std::max(1, hidden);
    seq->push_back(torch::nn::Linear(in, h));
    seq->push_back(torch::nn::Functional(torch::relu));
    in = h;
  }
  seq->push_back(torch::nn::Linear(in, n_actions * n_atoms));
  net = register_module("net", seq);
}

torch::Tensor C51ModelImpl::forward(torch::Tensor x) {
  auto logits = net->forward(x);
  return logits.view({x.size(0), n_actions, n_atoms});
}

C51Learner::C51Learner(const C51AgentConfig& config, int feature_size)
    : n_actions_(kActionCount),
      n_atoms_(std::max(2, config.n_atoms)),
      v_min_(config.v_min),
      v_max_(config.v_max),
      gamma_(config.gamma),
      learning_rate_(config.learning_rate),
      feature_size_(std::max(1, feature_size)) {
  if (v_max_ <= v_min_) {
    throw std::runtime_error("c51_agent v_max must be > v_min");
  }
  support_ = torch::linspace(v_min_, v_max_, n_atoms_);
  online_ = C51Model(feature_size_, n_actions_, n_atoms_, config.hidden_sizes);
  target_ = C51Model(feature_size_, n_actions_, n_atoms_, config.hidden_sizes);
  sync_target();
  optimizer_ = std::make_unique<torch::optim::Adam>(
      online_->parameters(), torch::optim::AdamOptions(learning_rate_));
}

int C51Learner::best_action(
    const std::array<float, kMaxFeatureSize>& features) {
  torch::NoGradGuard no_grad;
  auto x = torch::from_blob(const_cast<float*>(features.data()),
                            {1, feature_size_}, torch::kFloat32)
               .clone();
  auto logits = online_->forward(x);
  auto probs = torch::softmax(logits, /*dim=*/2);
  auto q = (probs * support_).sum(/*dim=*/2);
  return static_cast<int>(q.argmax(/*dim=*/1).item<int64_t>());
}

double C51Learner::train_step(const std::vector<Transition>& batch) {
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

  const double delta_z = (v_max_ - v_min_) / static_cast<double>(n_atoms_ - 1);

  torch::Tensor target_m;
  {
    torch::NoGradGuard no_grad;
    auto next_logits = target_->forward(next_features);
    auto next_probs = torch::softmax(next_logits, /*dim=*/2);
    auto next_q = (next_probs * support_).sum(/*dim=*/2);
    auto next_actions = next_q.argmax(/*dim=*/1);
    auto next_dist =
        next_probs
            .gather(/*dim=*/1,
                    next_actions.view({b, 1, 1}).expand({b, 1, n_atoms_}))
            .squeeze(1);

    auto Tz = rewards.unsqueeze(1) +
              (1.0 - dones.unsqueeze(1)) * gamma_ * support_.unsqueeze(0);
    Tz = Tz.clamp(v_min_, v_max_);
    auto b_idx = (Tz - v_min_) / delta_z;
    auto l = b_idx.floor().to(torch::kInt64);
    auto u = b_idx.ceil().to(torch::kInt64);
    auto eq = (l == u);
    l = l.clamp(0, n_atoms_ - 1);
    u = u.clamp(0, n_atoms_ - 1);

    target_m = torch::zeros({b, n_atoms_}, torch::kFloat32);
    auto offset = torch::arange(0, b, torch::kInt64).unsqueeze(1) * n_atoms_;
    auto m_flat = target_m.view({-1});
    auto next_flat = next_dist.view({-1});
    auto l_idx = (l + offset).view({-1});
    auto u_idx = (u + offset).view({-1});
    auto u_weight = (u.to(torch::kFloat32) - b_idx).view({-1});
    auto l_weight = (b_idx - l.to(torch::kFloat32)).view({-1});
    u_weight =
        torch::where(eq.view({-1}), torch::zeros_like(u_weight), u_weight);
    l_weight =
        torch::where(eq.view({-1}), torch::ones_like(l_weight), l_weight);
    m_flat.index_add_(0, l_idx, next_flat * u_weight);
    m_flat.index_add_(0, u_idx, next_flat * l_weight);
  }

  auto logits = online_->forward(features);
  auto log_probs = torch::log_softmax(logits, /*dim=*/2);
  auto chosen =
      log_probs
          .gather(/*dim=*/1, actions.view({b, 1, 1}).expand({b, 1, n_atoms_}))
          .squeeze(1);
  auto loss = -(target_m * chosen).sum(/*dim=*/1).mean();

  optimizer_->zero_grad();
  loss.backward();
  optimizer_->step();
  return loss.item<double>();
}

void C51Learner::sync_target() {
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

bool C51Learner::save(const std::string& path) const {
  if (path.empty()) {
    return false;
  }
  try {
    torch::serialize::OutputArchive archive;
    online_->save(archive);
    archive.write("n_atoms", torch::tensor({n_atoms_}));
    archive.write("v_min", torch::tensor({v_min_}));
    archive.write("v_max", torch::tensor({v_max_}));
    archive.save_to(path);
    return true;
  } catch (const std::exception& ex) {
    std::cerr << "c51_agent: save failed: " << ex.what() << '\n';
    return false;
  }
}

bool C51Learner::load(const std::string& path) {
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
    std::cerr << "c51_agent: load failed: " << ex.what() << '\n';
    return false;
  }
}

}  // namespace c51_agent
}  // namespace life_worlds
