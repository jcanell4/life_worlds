#include "life_worlds/agent/ql_agent/q_table.hpp"

#include <algorithm>
#include <fstream>

namespace life_worlds {
namespace ql_agent {

// Discretize distance in cells into one of four vision buckets.
int distance_bucket(int cells) {
  if (cells <= 1) {
    return 0;
  }
  if (cells <= 4) {
    return 1;
  }
  if (cells <= 10) {
    return 2;
  }
  return 3;
}

// Discretize current energy as a fraction of energy_max into four buckets.
int energy_bucket(int energy, int energy_max) {
  if (energy_max <= 0) {
    return 0;
  }
  const double ratio =
      static_cast<double>(std::max(0, energy)) /
      static_cast<double>(energy_max);
  if (ratio >= 0.75) {
    return 3;
  }
  if (ratio >= 0.50) {
    return 2;
  }
  if (ratio >= 0.25) {
    return 1;
  }
  return 0;
}

// Pack four vision rays and the energy bucket into a single sparse state key.
std::uint64_t encode_state(const Vision& vision, int energy_bucket_value) {
  const std::uint64_t per_dir =
      static_cast<std::uint64_t>(kDistanceBucketCount) *
      static_cast<std::uint64_t>(kObjectTypeCount);
  std::uint64_t key = 0;
  for (int d = 0; d < kDirectionCount; ++d) {
    const int bucket = std::clamp(vision[static_cast<std::size_t>(d)].distance, 0,
                                  kDistanceBucketCount - 1);
    const int type = std::clamp(
        vision[static_cast<std::size_t>(d)].object_type, 0, kObjectTypeCount - 1);
    const std::uint64_t digit =
        static_cast<std::uint64_t>(bucket * kObjectTypeCount + type);
    key = key * per_dir + digit;
  }
  const int energy = std::clamp(energy_bucket_value, 0, kEnergyBucketCount - 1);
  return key * static_cast<std::uint64_t>(kEnergyBucketCount) +
         static_cast<std::uint64_t>(energy);
}

// Return a mutable reference to Q(state, action), creating a zero row if needed.
double& QTable::value(std::uint64_t state, int action) {
  auto& row = table_[state];
  return row[static_cast<std::size_t>(std::clamp(action, 0, kActionCount - 1))];
}

// Read Q(state, action); unknown states are treated as 0.
double QTable::value(std::uint64_t state, int action) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0.0;
  }
  return it->second[static_cast<std::size_t>(
      std::clamp(action, 0, kActionCount - 1))];
}

// Choose the action with the largest Q-value for this state.
int QTable::best_action(std::uint64_t state) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0;
  }
  int best = 0;
  double best_value = it->second[0];
  for (int a = 1; a < kActionCount; ++a) {
    if (it->second[static_cast<std::size_t>(a)] > best_value) {
      best_value = it->second[static_cast<std::size_t>(a)];
      best = a;
    }
  }
  return best;
}

// Return max_a Q(state, a) used as the bootstrap target in Q-learning.
double QTable::max_value(std::uint64_t state) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0.0;
  }
  return *std::max_element(it->second.begin(), it->second.end());
}

// Apply one Q-learning update toward reward + gamma * max Q(next_state).
void QTable::update(std::uint64_t state, int action, double reward,
                    std::uint64_t next_state, double alpha, double gamma) {
  double& q = value(state, action);
  const double target = reward + gamma * max_value(next_state);
  q += alpha * (target - q);
}

// Serialize all (state, Q-row) pairs to a binary file.
bool QTable::save(const std::string& path) const {
  if (path.empty()) {
    return false;
  }
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }
  const std::uint64_t count = table_.size();
  out.write(reinterpret_cast<const char*>(&count), sizeof(count));
  for (const auto& [state, row] : table_) {
    out.write(reinterpret_cast<const char*>(&state), sizeof(state));
    out.write(reinterpret_cast<const char*>(row.data()),
              static_cast<std::streamsize>(sizeof(double) * row.size()));
  }
  return static_cast<bool>(out);
}

// Replace this table with the binary file contents at path.
bool QTable::load(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  std::uint64_t count = 0;
  in.read(reinterpret_cast<char*>(&count), sizeof(count));
  if (!in) {
    return false;
  }
  table_.clear();
  table_.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    std::uint64_t state = 0;
    Row row{};
    in.read(reinterpret_cast<char*>(&state), sizeof(state));
    in.read(reinterpret_cast<char*>(row.data()),
            static_cast<std::streamsize>(sizeof(double) * row.size()));
    if (!in) {
      table_.clear();
      return false;
    }
    table_.emplace(state, row);
  }
  return true;
}

}  // namespace ql_agent
}  // namespace life_worlds
