#pragma once

#include "life_worlds/agent/observation.hpp"
#include "life_worlds/agent/ql_agent/config.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace life_worlds {
namespace ql_agent {

// Encode rays (dist+type only) + energy bucket into a compact state key.
std::uint64_t encode_state(const agent::Vision& vision, int energy_bucket_value);

// Sparse Q(s,a) table shared by all agents of one QlAgentProcess.
class QTable {
 public:
  double& value(std::uint64_t state, int action);
  double value(std::uint64_t state, int action) const;
  int best_action(std::uint64_t state) const;
  double max_value(std::uint64_t state) const;
  void update(std::uint64_t state, int action, double reward,
              std::uint64_t next_state, double alpha, double gamma);
  bool save(const std::string& path) const;
  bool load(const std::string& path);
  std::size_t size() const { return table_.size(); }

 private:
  using Row = std::array<double, kActionCount>;
  std::unordered_map<std::uint64_t, Row> table_;
};

}  // namespace ql_agent
}  // namespace life_worlds
