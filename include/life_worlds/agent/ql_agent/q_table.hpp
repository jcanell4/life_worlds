#pragma once

#include "life_worlds/agent/ql_agent/config.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace life_worlds {
namespace ql_agent {

// One directional observation: discretized distance + first object hit.
struct VisionRay {
  // Discretized distance bucket (0 = near ... 3 = far), not raw cell count.
  int distance = 0;
  int object_type = static_cast<int>(ObjectType::EmptyEdge);
};

// Four orthogonal rays: Up, Down, Left, Right.
using Vision = std::array<VisionRay, kDirectionCount>;

// Map raw cell distance to a bucket: 1 -> 0, 2..4 -> 1, 5..10 -> 2, >10 -> 3.
int distance_bucket(int cells);

// Map energy level to a bucket:
// [75%,100%] -> 3, [50%,75%) -> 2, [25%,50%) -> 1, [0%,25%) -> 0.
int energy_bucket(int energy, int energy_max);

// Encode 4 rays + energy bucket into a compact state key (sparse Q-table).
std::uint64_t encode_state(const Vision& vision, int energy_bucket_value);

// Sparse Q(s,a) table shared by all agents of one QlAgentProcess.
class QTable {
 public:
  // Mutable Q-value for (state, action); inserts a zero row if missing.
  double& value(std::uint64_t state, int action);

  // Read-only Q-value; returns 0.0 when the state has never been seen.
  double value(std::uint64_t state, int action) const;

  // Greedy action with the highest Q-value (action 0 if the state is unknown).
  int best_action(std::uint64_t state) const;

  // Maximum Q-value over actions for a state (0.0 if unknown).
  double max_value(std::uint64_t state) const;

  // One-step Q-learning update: Q += alpha * (reward + gamma * max Q' - Q).
  void update(std::uint64_t state, int action, double reward,
              std::uint64_t next_state, double alpha, double gamma);

  // Write the sparse table to a binary file. Returns false on I/O failure.
  bool save(const std::string& path) const;

  // Replace the table with contents loaded from a binary file.
  bool load(const std::string& path);

  // Number of distinct states stored.
  std::size_t size() const { return table_.size(); }

 private:
  using Row = std::array<double, kActionCount>;
  std::unordered_map<std::uint64_t, Row> table_;
};

}  // namespace ql_agent
}  // namespace life_worlds
