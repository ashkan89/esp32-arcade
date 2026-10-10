#pragma once
#include "ShooterBase.h"
namespace arcade {
class SkyFighters final : public Shooter {
  void updateEnemies() override;

public:
  SkyFighters() : Shooter(ShooterMode::Sky) {}
};
} // namespace arcade
