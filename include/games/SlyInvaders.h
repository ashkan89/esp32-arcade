#pragma once
#include "ShooterBase.h"
namespace arcade {
class SlyInvaders final : public Shooter {
  void updateEnemies() override;

public:
  SlyInvaders() : Shooter(ShooterMode::Sly) {}
};
} // namespace arcade
