#pragma once

#include "life_worlds/agent/features.hpp"

#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace life_worlds {
namespace agent {

// One transition stored by neural agents (C51, DQN, …).
struct Transition {
  std::array<float, kMaxFeatureSize> features{};
  int action = 0;
  float reward = 0.0f;
  std::array<float, kMaxFeatureSize> next_features{};
  bool done = false;
};

// Circular buffer of transitions; uniform sampling for minibatch updates.
class ReplayBuffer {
 public:
  explicit ReplayBuffer(int capacity);

  void push(const Transition& transition);
  // Fill `batch` with `batch_size` samples (or fewer if the buffer is smaller).
  // Returns false when the buffer is empty.
  bool sample(int batch_size, std::vector<Transition>& batch,
              std::mt19937& rng) const;

  int size() const { return size_; }
  int capacity() const { return capacity_; }

 private:
  int capacity_ = 0;
  int size_ = 0;
  int next_ = 0;
  std::vector<Transition> data_;
};

}  // namespace agent
}  // namespace life_worlds
