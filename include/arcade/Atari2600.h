#pragma once
#include "arcade/Canvas.h"
#include "generated/KeystoneCpu.h"
#include <memory>
namespace arcade {
// Bounded 4K-cartridge runtime. CPU bus cycles drive RIOT and three TIA clocks.
class Atari2600 {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyKeystoneKapers();
#endif
  m6502_t cpu{};
  uint32_t pins = 0;
  std::array<uint8_t, 128> ram{};
  std::array<uint8_t, 64> tia{};
  std::array<uint8_t, 8> collisions{};
  std::array<int, 5> position{};
  std::array<uint8_t, 2> delayed{};
  std::unique_ptr<uint8_t[]> pixels;
  int hclock = 0, drawnClock = 0, scanline = 0, timer = 0, divider = 1, prescale = 1;
  bool waiting = false, timerFlag = false, delayedBall = false, resetSwitch = false;
  uint16_t buttons = 0;
  uint32_t frames = 0;
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);
  void flushPixels();
  void advanceClocks(int cycles);
public:
  static constexpr int Width = 160, Height = 210;
  bool reset();
  bool frame(uint16_t held, bool reset = false);
  void render(Canvas &canvas) const;
  uint8_t memory(uint8_t address) const { return ram[address & 127]; }
  uint32_t frameCount() const { return frames; }
  uint8_t reg(unsigned address) const { return tia[address & 63]; }
  void release() { pixels.reset(); }
  uint16_t pc() const { return m6502_pc(const_cast<m6502_t *>(&cpu)); }
};
} // namespace arcade
