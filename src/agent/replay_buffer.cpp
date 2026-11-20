#include "life_worlds/agent/replay_buffer.hpp"

#include <algorithm>

namespace life_worlds {
namespace agent {

ReplayBuffer::ReplayBuffer(int capacity)
    : capacity_(std::max(1, capacity)), data_(static_cast<std::size_t>(capacity_)) {}

void ReplayBuffer::push(const Transition& transition) {
  data_[static_cast<std::size_t>(next_)] = transition;
  next_ = (next_ + 1) % capacity_;
  size_ = std::min(size_ + 1, capacity_);
}

bool ReplayBuffer::sample(int batch_size, std::vector<Transition>& batch,
                          std::mt19937& rng) const {
  if (size_ <= 0) {
    return false;
  }
  const int n = std::min(batch_size, size_);
  batch.resize(static_cast<std::size_t>(n));
  std::uniform_int_distribution<int> pick(0, size_ - 1);
  for (int i = 0; i < n; ++i) {
    batch[static_cast<std::size_t>(i)] =
        data_[static_cast<std::size_t>(pick(rng))];
  }
  return true;
}

}  // namespace agent
}  // namespace life_worlds
