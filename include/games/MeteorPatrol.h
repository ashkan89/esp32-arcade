#pragma once
#include "ShooterBase.h"
namespace arcade {
class MeteorPatrol final : public Shooter {
  void updateEnemies() override;

public:
  MeteorPatrol() : Shooter(ShooterMode::Meteor) {}
};
} // namespace arcade
