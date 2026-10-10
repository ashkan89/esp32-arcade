#pragma once
#include "ShooterBase.h"
namespace arcade {
class StarAssault final : public Shooter {
  void updateEnemies() override;

public:
  StarAssault() : Shooter(ShooterMode::Star) {}
};
} // namespace arcade
