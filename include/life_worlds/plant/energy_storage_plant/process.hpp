#pragma once

#include "life_worlds/entity/entity.hpp"
#include "life_worlds/plant/energy_storage_plant/config.hpp"
#include "life_worlds/ports/occupancy_port.hpp"

#include <cstdint>

namespace life_worlds {
namespace energy_storage_plant {

// Plants that store energy every cycle and split into an adjacent free cell
// when full. Descendants share the founder's lineage, which has a cell cap.
class EnergyStoragePlantProcess final : public Entity {
 public:
  EnergyStoragePlantProcess(EnergyStoragePlantConfig config,
                            OccupancyPort occupancy);

  void step(StepContext& ctx) override;

  const EnergyStoragePlantConfig& config() const { return config_; }

 private:
  EnergyStoragePlantConfig config_;
  OccupancyPort occupancy_;
  bool seeded_ = false;
  std::uint32_t next_lineage_id_ = 0;
};

}  // namespace energy_storage_plant
}  // namespace life_worlds
