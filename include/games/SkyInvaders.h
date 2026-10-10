#pragma once
#include "ShooterBase.h"
namespace arcade {
class SkyInvaders final : public Shooter {
  void updateEnemies() override;

public:
  SkyInvaders() : Shooter(ShooterMode::Invaders) {}
};
} // namespace arcade
