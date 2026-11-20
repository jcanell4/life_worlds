#include "life_worlds/agent/ql_agent/q_table.hpp"

#include <algorithm>
#include <fstream>

namespace life_worlds {
namespace ql_agent {

std::uint64_t encode_state(const agent::Vision& vision,
                           int energy_bucket_value) {
  using agent::kDistanceBucketCount;
  using agent::kEnergyBucketCount;
  using agent::kMaxVisionRays;
  using agent::kObjectTypeCount;

  const std::uint64_t per_dir =
      static_cast<std::uint64_t>(kDistanceBucketCount) *
      static_cast<std::uint64_t>(kObjectTypeCount);
  std::uint64_t key = 0;
  const int rays = std::clamp(vision.ray_count, 1, kMaxVisionRays);
  for (int d = 0; d < rays; ++d) {
    const int bucket = std::clamp(vision[static_cast<std::size_t>(d)].distance, 0,
                                  kDistanceBucketCount - 1);
    const int type = std::clamp(vision[static_cast<std::size_t>(d)].object_type,
                                0, kObjectTypeCount - 1);
    const std::uint64_t digit =
        static_cast<std::uint64_t>(bucket * kObjectTypeCount + type);
    key = key * per_dir + digit;
  }
  const int energy = std::clamp(energy_bucket_value, 0, kEnergyBucketCount - 1);
  return key * static_cast<std::uint64_t>(kEnergyBucketCount) +
         static_cast<std::uint64_t>(energy);
}

double& QTable::value(std::uint64_t state, int action) {
  auto& row = table_[state];
  return row[static_cast<std::size_t>(std::clamp(action, 0, kActionCount - 1))];
}

double QTable::value(std::uint64_t state, int action) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0.0;
  }
  return it->second[static_cast<std::size_t>(
      std::clamp(action, 0, kActionCount - 1))];
}

int QTable::best_action(std::uint64_t state) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0;
  }
  int best = 0;
  double best_value = it->second[0];
  for (int a = 1; a < kActionCount; ++a) {
    const double v = it->second[static_cast<std::size_t>(a)];
    if (v > best_value) {
      best_value = v;
      best = a;
    }
  }
  return best;
}

double QTable::max_value(std::uint64_t state) const {
  const auto it = table_.find(state);
  if (it == table_.end()) {
    return 0.0;
  }
  double best = it->second[0];
  for (int a = 1; a < kActionCount; ++a) {
    best = std::max(best, it->second[static_cast<std::size_t>(a)]);
  }
  return best;
}

void QTable::update(std::uint64_t state, int action, double reward,
                    std::uint64_t next_state, double alpha, double gamma) {
  double& q = value(state, action);
  const double target = reward + gamma * max_value(next_state);
  q += alpha * (target - q);
}

bool QTable::save(const std::string& path) const {
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

bool QTable::load(const std::string& path) {
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
    table_[state] = row;
  }
  return true;
}

}  // namespace ql_agent
}  // namespace life_worlds
