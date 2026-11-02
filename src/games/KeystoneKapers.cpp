#include "arcade/Atari2600.h"
#include "games/Registration.h"
namespace arcade {
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
void reportVcsProfile();
#endif
class KeystoneKapers final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyKeystoneKapers();
#endif
  Atari2600 machine;
  bool ready = false;
  uint8_t lastVolume = 0;
  static unsigned bcd(uint8_t value) { return (value >> 4) * 10 + (value & 15); }
  void simulate() override {
    if (!ready) return;
    if (!machine.frame(input.held)) { ended = true; sound(Sound::Over); return; }
    // FDB6/FDB8 add packed-BCD score at 9A:9B:9C; FA37 initializes timer 9D.
    uint32_t score = bcd(machine.memory(0x9a)) * 10000 +
                     bcd(machine.memory(0x9b)) * 100 + bcd(machine.memory(0x9c));
    points = std::max(points, std::min(uint32_t(999999), score));
    health = machine.memory(0x96) + 1;
    level = uint16_t(machine.memory(0x97)) + 1;
    uint8_t volume = (machine.reg(0x19) | machine.reg(0x1a)) & 15;
    if (volume && !lastVolume) sound(Sound::Footstep);
    lastVolume = volume;
    // 88 bit 7 denotes active play; the cartridge clears it on game over.
    if (tick > 2 && !(machine.memory(0x88) & 0x80)) { health = 0; ended = true; sound(Sound::Over); }
  }
public:
  void start(const GameOptions &o) override {
    baseStart(o);
    ready = machine.reset();
    if (!ready) { ended = true; return; }
    machine.frame(0); machine.frame(0, true); machine.frame(0);
    points = 0; health = machine.memory(0x96) + 1; lastVolume = 0;
  }
  void cleanup() override {
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
    if (ready) reportVcsProfile();
#endif
    Game::cleanup(); machine.release(); ready = false;
  }
  void render(Canvas &c) const override { machine.render(c); }
  size_t activeEntities() const override { return ready ? 8 : 0; }
  bool ownsHud() const override { return true; }
  unsigned maxCatchUpSteps() const override { return 1; }
  uint16_t stageLimit() const override { return 0; }
};
GameEntry describeKeystoneKapers() {
  return gameEntry<KeystoneKapers>(
      {"KEYSTONE KAPERS", "ORIGINAL 4K CARTRIDGE",
       {"L/R RUN  A JUMPS", "DOWN DUCKS UNDER PLANES", "UP ENTERS OPEN LIFT",
        "DOWN EXITS LIFT AT STOP", "ESCALATORS ONLY GO UP", "CATCH KROOK BEFORE 50 SEC"},
       Cyan, 50, Left | Right | Up | Down | Fire});
}
#ifdef ARCADE_HOST_TESTS
bool verifyKeystoneKapers() {
  KeystoneKapers g; g.start({});
  HOST_CHECK(g.ready && !g.ended);
  HOST_CHECK(g.maxCatchUpSteps() == 1);
  std::array<uint16_t, W * H> pixels{};
  Canvas c; c.attach(pixels.data());
  g.render(c);
  HOST_CHECK(captureAtari(c, "keystone_rom_00_start"));
  std::fprintf(stdout, "KEYSTONE boot PC=%04x state=%02x lives=%02x timer=%02x\n", g.machine.pc(), g.machine.memory(0x88), g.machine.memory(0x96), g.machine.memory(0x9d));
  HOST_CHECK(g.machine.memory(0x9d) == 0x50);
  uint8_t initialX = g.machine.memory(0xa0);
  auto initial = pixels;
  for (int t = 0; t < 120; ++t) { g.handle({Right, 0}); g.update(); }
  g.render(c);
  HOST_CHECK(!g.ended && g.machine.memory(0xa0) > initialX);
  HOST_CHECK(pixels != initial);
  HOST_CHECK(captureAtari(c, "keystone_rom_01_run"));
  g.handle({Fire,Fire}); g.update();
  HOST_CHECK(g.machine.memory(0x8d) != 0); // cartridge's jump countdown
  for (int t = 0; t < 29; ++t) { g.handle({Right | Fire, 0}); g.update(); }
  g.render(c);
  HOST_CHECK(captureAtari(c, "keystone_rom_02_jump"));
  HOST_CHECK(g.machine.memory(0x9d) < 0x50);
  uint32_t frame = g.machine.frameCount();
  g.pause(); g.update(); HOST_CHECK(g.machine.frameCount() == frame);
  g.resume(); g.update(); HOST_CHECK(g.machine.frameCount() == frame + 1);
  g.start({}); HOST_CHECK(g.points == 0 && !g.ended);
  // A six-digit cartridge score must survive the console adapter intact.
  // Pickups use the low byte 9C, while time bonuses enter at 9B.
  g.machine.ram[0x9a & 127] = 0x12;
  g.machine.ram[0x9b & 127] = 0x34;
  g.machine.ram[0x9c & 127] = 0x56;
  g.handle({}); g.update(); HOST_CHECK(g.points == 123456);
  g.cleanup(); HOST_CHECK(!g.ready && !g.machine.pixels);
  // Fixtures captured from the unaccelerated, cycle-by-cycle runtime. These
  // catch off-by-one WSYNC releases and timer-poll skips that shift the beam.
  Atari2600 trace; HOST_CHECK(trace.reset());
  Random traceInput;
  uint16_t held = 0;
  const int frames[] = {31,120,499};
  const uint32_t ramCrc[] = {0x1c45bfcb,0x449626f5,0x52b6da00};
  const uint32_t pixelCrc[] = {0x7b7701c3,0xf254baa8,0xe248cd88};
  unsigned fixture = 0;
  for (int frame = 0; frame <= 499; ++frame) {
    if (frame % 25 == 0) held = uint16_t(traceInput.next() & (Left|Right|Up|Down|Fire));
    HOST_CHECK(trace.frame(held,frame == 1));
    if (frame == frames[fixture]) {
      std::array<uint8_t,128> ram{};
      for (unsigned i = 0; i < ram.size(); ++i) ram[i] = trace.memory(uint8_t(i));
      trace.render(c);
      HOST_CHECK(checksum(ram.data(),ram.size()) == ramCrc[fixture]);
      HOST_CHECK(checksum(reinterpret_cast<const uint8_t *>(pixels.data()),pixels.size()*2) == pixelCrc[fixture]);
      HOST_CHECK(trace.pc() == 0xf3cc && trace.hclock == 3 && trace.scanline == 0);
      ++fixture;
    }
  }
  return true;
}
#endif
} // namespace arcade
