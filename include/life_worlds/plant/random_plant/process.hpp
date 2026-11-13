#pragma once

#include "life_worlds/entity/entity.hpp"
#include "life_worlds/plant/random_plant/config.hpp"
#include "life_worlds/ports/occupancy_port.hpp"

namespace life_worlds {
namespace random_plant {

// Ages, kills, reproduces, and reseeds plant populations each tick.
class RandomPlantProcess final : public Entity {
 public:
  RandomPlantProcess(RandomPlantConfig config, OccupancyPort occupancy);

  void step(StepContext& ctx) override;

  const RandomPlantConfig& config() const { return config_; }

 private:
  RandomPlantConfig config_;
  OccupancyPort occupancy_;
};

}  // namespace random_plant
}  // namespace life_worlds
