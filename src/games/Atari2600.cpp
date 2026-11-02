// Throughput is critical to a cycle-stepped CPU; apply this only to the runtime.
#if defined(__GNUC__) && defined(ARDUINO_ARCH_ESP32)
#pragma GCC optimize ("O3")
#if defined(ARCADE_WROOM) && ARCADE_WROOM
#include <esp_attr.h>
// The generated dispatcher exceeds the instruction cache. Compact IRAM code
// avoids cache thrashing between the CPU, cartridge bus and raster routines.
// WROVER reserves more IRAM for its PSRAM workaround, so retain flash there.
#define m6502_tick IRAM_ATTR __attribute__((optimize("O2"))) m6502_tick
#endif
#endif
#define CHIPS_IMPL
#include "arcade/Atari2600.h"
#ifdef m6502_tick
#undef m6502_tick
#endif
#include "generated/KeystoneRom.h"
#include <cstring>
#include <new>
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
#include <Arduino.h>
#endif
namespace arcade {
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
namespace {
inline uint32_t cycleCounter() { uint32_t v; asm volatile("rsr %0, ccount" : "=a"(v)); return v; }
uint32_t rasterCycles = 0, maxCpu = 0, maxRaster = 0, maxTotal = 0, lastTicks = 0;
struct RasterSample {
  uint32_t begin = cycleCounter();
  ~RasterSample() { rasterCycles += cycleCounter()-begin; }
};
}
void reportVcsProfile() {
  Serial.printf("VCS_PROFILE totalMax=%lu cpuMax=%lu rasterMax=%lu ticks=%lu\n",
    (unsigned long)(maxTotal/240),(unsigned long)(maxCpu/240),(unsigned long)(maxRaster/240),(unsigned long)lastTicks);
  rasterCycles = maxCpu = maxRaster = maxTotal = lastTicks = 0;
}
#endif
bool Atari2600::reset() {
  if (!pixels) pixels.reset(new (std::nothrow) uint8_t[Width * Height]);
  if (!pixels) return false;
  std::memset(pixels.get(), 0, Width * Height);
  ram.fill(0); tia.fill(0); collisions.fill(0); position.fill(0); delayed.fill(0);
  hclock = drawnClock = scanline = timer = 0; divider = prescale = 1;
  waiting = timerFlag = delayedBall = resetSwitch = false;
  frames = 0; buttons = 0;
  m6502_desc_t desc{};
  pins = m6502_init(&cpu, &desc);
  return true;
}
uint8_t Atari2600::read(uint16_t a) {
  a &= 0x1fff;
  if (a & 0x1000) return KeystoneRom[a & 4095];
  if ((a & 0x1280) == 0x80) return ram[a & 127];
  if (a & 0x200) {
    switch (a & 7) {
    case 0: return uint8_t(0xff ^ ((buttons & Right ? 0x80 : 0) | (buttons & Left ? 0x40 : 0) |
                                  (buttons & Down ? 0x20 : 0) | (buttons & Up ? 0x10 : 0)));
    case 2: return resetSwitch ? 0xfe : 0xff; // color, B/B difficulty
    case 4: timerFlag = false; return uint8_t(timer);
    case 5: return timerFlag ? 0x80 : 0;
    default: return 0xff;
    }
  }
  unsigned r = a & 15;
  if (r < 8) { flushPixels(); return collisions[r]; }
  if (r == 12) return buttons & Fire ? 0 : 0x80;
  return 0x80;
}
void Atari2600::write(uint16_t a, uint8_t v) {
  a &= 0x1fff;
  if (a & 0x1000) return;
  if ((a & 0x1280) == 0x80) { ram[a & 127] = v; return; }
  if (a & 0x200) {
    if ((a & 0x14) == 0x14) {
      static constexpr int scales[] = {1, 8, 64, 1024};
      timer = v; divider = prescale = scales[a & 3]; timerFlag = false;
    }
    return;
  }
  a &= 63;
  // Resolve the elapsed span before a register changes the beam's output.
  flushPixels();
  if (a == 0 && (v & 2) && !(tia[0] & 2)) { ++frames; scanline = 0; }
  if (a == 2) waiting = true;
  if (a == 3) hclock = drawnClock = 0;
  if (a >= 0x10 && a <= 0x14) {
    int offset = a <= 0x11 ? 5 : 4;
    position[a - 0x10] = hclock < 68 ? offset - 2 : (hclock - 68 + offset) % 160;
  }
  if (a == 0x1b) delayed[1] = tia[0x1c];
  if (a == 0x1c) { delayed[0] = tia[0x1b]; delayedBall = tia[0x1f] & 2; }
  if (a == 0x2a) {
    for (int n = 0; n < 5; ++n) {
      int move = int8_t(tia[0x20 + n]) >> 4;
      position[n] = (position[n] - move + 160) % 160;
    }
  }
  if (a == 0x2b) for (int n = 0; n < 5; ++n) tia[0x20+n] = 0;
  if (a == 0x2c) collisions.fill(0);
  tia[a] = v;
}
void Atari2600::flushPixels() {
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
  RasterSample sample;
#endif
  int begin = clamp(drawnClock - 68, 0, 160);
  int end = clamp(hclock - 68, 0, 160);
  drawnClock = hclock;
  if (end <= begin) return;
  int y = scanline - 35;
  uint8_t *out = y >= 0 && y < Height ? pixels.get() + y * Width : nullptr;
  if ((tia[0] | tia[1]) & 2) {
    if (out) std::memset(out + begin, 0, size_t(end - begin));
    return;
  }
  // TIA registers are constant over this span. Expand only the playfield's
  // four-clock cells and the objects' lit bits, rather than recomputing every
  // object's modulo/copy/scale tests on all 160 pixels every scanline.
  uint8_t presence[160];
  for (int x = begin; x < end;) {
    int stop = std::min(end, (x / 4 + 1) * 4);
    int cell = x / 4;
    if (cell >= 20) cell = tia[10] & 1 ? 39 - cell : cell - 20;
    bool pf = cell < 4 ? tia[13] & (0x10 << cell) :
              cell < 12 ? tia[14] & (0x80 >> (cell - 4)) : tia[15] & (1 << (cell - 12));
    std::memset(presence+x, pf ? 32 : 0, size_t(stop-x));
    uint8_t color = pf ? ((tia[10] & 2) ? tia[x < 80 ? 6 : 7] : tia[8]) : tia[9];
    if (out) std::memset(out+x, color >> 1, size_t(stop-x));
    x = stop;
  }
  uint8_t hit[5] = {};
  bool any = false;
  auto paint = [&](int object, int start, int width) {
    start %= 160;
    for (int i = 0; i < width; ++i) {
      int x = start+i;
      if (x >= 160) x -= 160;
      if (x < begin || x >= end) continue;
      hit[object] |= presence[x];
      presence[x] |= uint8_t(1 << object);
      any = true;
    }
  };
  auto copies = [](unsigned size, int *offsets) {
    int n = 1; offsets[0] = 0;
    if (size == 1 || size == 3) offsets[n++] = 16;
    if (size == 2 || size == 3 || size == 6) offsets[n++] = 32;
    if (size == 4 || size == 6) offsets[n++] = 64;
    return n;
  };
  for (int n = 0; n < 2; ++n) {
    uint8_t graphics = tia[0x25+n] & 1 ? delayed[n] : tia[0x1b+n];
    if (!graphics) continue;
    unsigned size = tia[4+n] & 7;
    int scale = size == 5 ? 2 : size == 7 ? 4 : 1;
    int offsets[3]; int count = copies(size,offsets);
    for (int copy = 0; copy < count; ++copy)
      for (int bit = 0; bit < 8; ++bit)
        if (graphics & (tia[0x0b+n] & 8 ? 1 << bit : 0x80 >> bit))
          paint(n,position[n]+offsets[copy]+bit*scale,scale);
  }
  for (int n = 0; n < 2; ++n) {
    if (!(tia[0x1d+n] & 2)) continue;
    int origin = tia[0x28+n] & 2 ? (position[n]+4)%160 : position[2+n];
    int offsets[3]; int count = copies(tia[4+n] & 7,offsets);
    for (int copy = 0; copy < count; ++copy)
      paint(2+n,origin+offsets[copy],1 << ((tia[4+n] >> 4) & 3));
  }
  if (tia[0x27] & 1 ? delayedBall : tia[0x1f] & 2)
    paint(4,position[4],1 << ((tia[10] >> 4) & 3));
  if (hit[2] & 2) collisions[0] |= 0x80;
  if (hit[2] & 1) collisions[0] |= 0x40;
  if (hit[3] & 1) collisions[1] |= 0x80;
  if (hit[3] & 2) collisions[1] |= 0x40;
  for (int n = 0; n < 4; ++n) {
    if (hit[n] & 32) collisions[n+2] |= 0x80;
    if (hit[4] & (1 << n)) collisions[n+2] |= 0x40;
  }
  if (hit[4] & 32) collisions[6] |= 0x80;
  if (hit[1] & 1) collisions[7] |= 0x80;
  if (hit[3] & 4) collisions[7] |= 0x40;
  if (out && any) for (int x = begin; x < end; ++x) {
    unsigned mask = presence[x];
    if (!(mask & 31)) continue;
    uint8_t color = tia[9];
    if (mask & 48) color = (mask & 32) && !(mask & 16) && (tia[10] & 2) ? tia[x < 80 ? 6 : 7] : tia[8];
    if (!(tia[10] & 4) || !(mask & 48)) {
      if (mask & 10) color = tia[7];
      if (mask & 5) color = tia[6];
    }
    out[x] = color >> 1;
  }
}
void Atari2600::advanceClocks(int cycles) {
  while (cycles > 0) {
    int step = std::min(cycles, (228-hclock)/3);
    int remaining = step;
    while (remaining >= prescale) {
      remaining -= prescale;
      if (--timer < 0) { timer = 255; timerFlag = true; divider = 1; }
      prescale = divider;
    }
    prescale -= remaining;
    hclock += step * 3;
    if (hclock == 228) {
      flushPixels(); hclock = drawnClock = 0; ++scanline; waiting = false;
    }
    cycles -= step;
  }
}
bool Atari2600::frame(uint16_t held, bool reset) {
  if (!pixels) return false;
  buttons = held; resetSwitch = reset;
  uint32_t previous = frames;
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
  uint32_t beginFrame = cycleCounter(), cpuCycles = 0, beginRaster = rasterCycles, ticks = 0;
#endif
  // A broken cartridge cannot block the UI/watchdog indefinitely.
  for (int cycle = 0; cycle < 24000; ++cycle) {
    // WSYNC holds a read cycle until horizontal blank. There are no external
    // IRQ/NMI pins on the 6507, so repeated stalled CPU ticks add no state.
    if (waiting && (pins & M6502_RW)) {
      // The boundary cycle itself releases RDY and advances the CPU.
      int skip = std::min((228-hclock)/3 - 1, 24000-cycle);
      if (skip > 0) { advanceClocks(skip); cycle += skip-1; continue; }
    }
    // This verified cartridge polls INTIM with LDX/LDA abs; BNE back (7 cycles).
    // Skip complete non-final iterations; execute the terminating poll normally.
    // TIA, RIOT and collision clocks continue exactly during the skipped CPU work.
    uint16_t pc = M6502_GET_ADDR(pins);
    if ((pins & M6502_SYNC) && (pc == 0xf3c1 || pc == 0xf6d4) && timer > 1) {
      int skip = (prescale + (timer-1)*divider) / 7 * 7;
      skip = std::min(skip, (24000-cycle)/7*7);
      if (skip >= 7) { advanceClocks(skip); cycle += skip-1; continue; }
    }
    if (--prescale <= 0) {
      prescale = divider;
      if (--timer < 0) { timer = 255; timerFlag = true; divider = prescale = 1; }
    }
    hclock += 3;
    if (hclock == 228) {
      flushPixels(); hclock = drawnClock = 0; ++scanline; waiting = false;
    }
    if (waiting) pins |= M6502_RDY; else pins &= ~M6502_RDY;
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
    uint32_t atCpu = cycleCounter();
#endif
    pins = m6502_tick(&cpu, pins);
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
    cpuCycles += cycleCounter()-atCpu; ++ticks;
#endif
    uint16_t a = M6502_GET_ADDR(pins);
    if (pins & M6502_RW) { uint8_t value = read(a); M6502_SET_DATA(pins, value); }
    else write(a, M6502_GET_DATA(pins));
    if (frames != previous) {
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
      maxCpu = std::max(maxCpu,cpuCycles); maxRaster = std::max(maxRaster,rasterCycles-beginRaster);
      maxTotal = std::max(maxTotal,cycleCounter()-beginFrame); lastTicks = ticks;
#endif
      return true;
    }
  }
  return false;
}
void Atari2600::render(Canvas &c) const {
  c.clear();
  if (!pixels) { c.centered(90, "ROM MEMORY UNAVAILABLE", Red); return; }
  // NTSC hue/luminance approximation; cartridge values remain intact in pixels.
  static constexpr uint16_t hues[] = {0xffff,0xff60,0xfce0,0xfba8,0xf9af,0xc1b9,0x89bf,0x329f,
                                     0x045f,0x067c,0x0753,0x6726,0x9ee4,0xcde3,0xdce5,0xe3a8};
  uint16_t palette[128];
  for (int n = 0; n < 128; ++n) {
    unsigned lum = n & 7, hue = n >> 3;
    unsigned base = hues[hue];
    unsigned r = ((base >> 11) & 31) * (lum + 1) / 8;
    unsigned g = ((base >> 5) & 63) * (lum + 1) / 8;
    unsigned b = (base & 31) * (lum + 1) / 8;
    palette[n] = n < 8 ? uint16_t((lum * 31 / 7) << 11 | (lum * 63 / 7) << 5 | lum * 31 / 7) : uint16_t(r << 11 | g << 5 | b);
  }
  for (int y = 0; y < Height; ++y)
    for (int x = 0; x < Width; ++x) c.pixel(8+x, 5+y, palette[pixels[y * Width+x]]);
}
} // namespace arcade
