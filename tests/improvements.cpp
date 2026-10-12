#include "arcade/AudioSequencer.h"
#include "arcade/Behavior.h"
#include "arcade/Persistence.h"
#include "games/Campaign.h"
#include <cstdio>
#include <cstdlib>
namespace arcade {
#define ARCADE_GAME(id, type, number) GameEntry describe##type();
#include "games/Catalog.def"
#undef ARCADE_GAME
static void require(bool ok, const char *what, unsigned seed = 0, unsigned game = 0,
                    unsigned tick = 0) {
  if (!ok) {
    std::fprintf(stderr, "IMPROVEMENT FAIL %s seed=%u game=%u tick=%u\n", what, seed, game, tick);
    std::exit(1);
  }
}
void verifyImprovements() {
  // Count actual writer calls, including failed calls; simulated time needs no Arduino.
  DeferredSave<SaveBytes> save;
  Settings savedSettings;
  SaveBytes original = encode(savedSettings), written{};
  unsigned writes = 0;
  bool failWrite = false;
  auto writer = [&](const SaveBytes &bytes) {
    ++writes;
    if (failWrite)
      return false;
    written = bytes;
    return true;
  };
  save.remember(original);
  require(save.update(original, 100000, true, writer) == SaveResult::Deferred && writes == 0,
          "idle boot does not save defaults or loaded state");
  savedSettings.sound ^= 1;
  SaveBytes modified = encode(savedSettings);
  save.mark(100);
  require(save.update(modified, 2099, true, writer) == SaveResult::Deferred,
          "save waits for quiet period");
  require(save.update(modified, 2100, false, writer) == SaveResult::Deferred && writes == 0,
          "gameplay and attract never write");
  require(save.update(modified, 2100, true, writer) == SaveResult::Saved && writes == 1 &&
              written == modified, "first changed state is saved");
  save.mark(2200);
  require(save.update(modified, 4200, true, writer) == SaveResult::Deferred && writes == 1,
          "duplicate dirty notification waits without writing");
  save.mark(5000);
  require(save.update(original, 7000, true, writer) == SaveResult::Deferred && writes == 1,
          "changed state waits through minute cooldown");
  save.mark(8000);
  require(save.update(modified, 10000, true, writer) == SaveResult::Deferred && writes == 1,
          "cooldown avoids repeated snapshot work");
  require(save.update(modified, 62100, true, writer) == SaveResult::Unchanged && writes == 1,
          "reverted change cancels pending save when eligible");
  save.mark(62200);
  save.mark(63000);
  require(save.update(original, 64999, true, writer) == SaveResult::Deferred,
          "latest edit extends debounce");
  require(save.update(original, 65000, true, writer) == SaveResult::Saved && writes == 2,
          "coalesced state saved after cooldown and debounce");
  failWrite = true;
  save.mark(66000);
  require(save.update(modified, 125000, true, writer) == SaveResult::Failed && writes == 3,
          "failed write retains pending state");
  require(save.update(modified, 184999, true, writer) == SaveResult::Deferred && writes == 3,
          "failed write cannot cause retry storm");
  failWrite = false;
  savedSettings.music ^= 1;
  SaveBytes newest = encode(savedSettings);
  save.mark(180000);
  require(save.update(newest, 185000, true, writer) == SaveResult::Saved && writes == 4 &&
              written == newest, "retry persists newest state, not failed snapshot");
  DeferredSave<SaveBytes> migration;
  migration.mark(10);
  require(migration.update(original, 2010, true, writer) == SaveResult::Saved && writes == 5,
          "migration without current-format snapshot saves once");
  require(migration.update(original, 100000, true, writer) == SaveResult::Deferred && writes == 5,
          "migration does not repeat");
  DeferredSave<SaveBytes> uncertain;
  uncertain.remember(original);
  uncertain.mark(0);
  auto failedCommit = [&](const SaveBytes &bytes) {
    written = bytes;
    return false;
  };
  require(uncertain.update(modified, 2000, true, failedCommit) == SaveResult::Failed,
          "failed commit may have changed physical state");
  uncertain.mark(3000);
  require(uncertain.update(original, 62000, true, writer) == SaveResult::Saved && written == original,
          "revert after uncertain commit performs compensating save");
  DeferredSave<SaveBytes> wrapSave;
  wrapSave.remember(original);
  wrapSave.mark(0xfffffff0u);
  require(wrapSave.update(modified, 1984, true, writer) == SaveResult::Saved,
          "debounce crosses millis rollover");
  WriteCooldown cooldown;
  require(cooldown.eligible(0), "first explicit network save allowed at time zero");
  cooldown.noteAttempt(0xfffffff0u);
  require(!cooldown.eligible(59983) && cooldown.eligible(59984),
          "write cooldown crosses millis rollover at exact boundary");
  std::puts("PASS persistence write counts, debounce, coalescing, revert, retries, migration and rollover");
  size_t maximumGame = 0;
#define ARCADE_GAME(id, type, number) maximumGame = std::max(maximumGame, describe##type().bytes);
#include "games/Catalog.def"
#undef ARCADE_GAME
  require(maximumGame <= 8192, "catalog game arena budget");
  std::printf("BUDGET maxGame=%zu/8192 Settings=%zu Save=%zu AudioSequencer=%zu bytes\n",
              maximumGame, sizeof(Settings), SaveSize, sizeof(AudioSequencer));
  AudioSequencer a, reference;
  a.select(MenuTrack, 0);
  reference.select(MenuTrack, 0);
  require(a.update(0) > 0, "menu has pitch");
  a.play(Sound::Shot, 0);
  require(a.activeEffect() == Sound::Shot, "shot begins");
  a.play(Sound::Warning, 1);
  require(a.activeEffect() == Sound::Warning, "critical preempts");
  a.play(Sound::Warning, 2);
  require(a.coalesced == 1, "duplicate coalesces");
  for (unsigned ms = 2; ms <= 240; ++ms) {
    a.update(ms);
    reference.update(ms);
  }
  require(a.musicalPosition() == reference.musicalPosition(), "transport survives interruption");
  require(a.update(241) == reference.update(241), "resume at current music note");
  auto at = a.musicalPosition();
  a.setPaused(true, 241);
  require(a.update(1000) == 0 && a.musicalPosition() == at, "paused transport freezes");
  a.setPaused(false, 1000);
  a.update(1100);
  require(a.musicalPosition() == at + 100, "resume excludes pause duration");
  a.configure(false, true, true);
  require(a.update(1101) == 0, "mute silences");
  a.configure(true, false, true);
  require(a.update(1102) == 0, "music disable silences");
  a.play(Sound::Power, 1102);
  require(a.update(1102) > 0, "SFX independent of music");
  a.configure(true, true, false);
  require(a.activeEffect() == Sound::None, "SFX off clears effects");
  a.stop(1103);
  require(a.update(1103) == 0 && !a.queued(), "exit clears output and queue");
  AudioSequencer q;
  q.select(MenuTrack, 0);
  q.play(Sound::Victory, 0);
  for (auto s : {Sound::Move, Sound::Shot, Sound::Select, Sound::Power, Sound::BackCue, Sound::Hit})
    q.play(s, 0);
  require(q.queued() == 4 && q.dropped > 0, "bounded priority queue overflow");
  q.update(1000);
  require(q.queued() == 0, "stale cues expire");
  AudioSequencer wrap;
  wrap.select(MenuTrack, 0xfffffff0u);
  wrap.update(20);
  require(wrap.musicalPosition() == 36, "audio wraparound");
  AudioSequencer late;
  late.select(MenuTrack, 0);
  late.update(10000000);
  require(late.musicalPosition() == 10000000, "bounded large stall transport");
  AudioSequencer gun;
  gun.select(MenuTrack, 0);
  unsigned musicFrames = 0;
  for (unsigned ms = 0; ms < 1000; ++ms) {
    if (ms % 16 == 0)
      gun.play(Sound::Shot, ms);
    gun.update(ms);
    musicFrames += gun.activeEffect() == Sound::None;
  }
  require(musicFrames > 500, "gunfire cannot starve music");
  for (unsigned id = 0; id <= GameCount; ++id) {
    auto &t = audioTrack(id);
    require(t.count >= 8 && t.beatMs >= 300 && t.name && *t.name, "catalog track mapping", 0, id);
    bool rest = false;
    for (unsigned n = 0; n < t.count; ++n)
      rest |= !t.notes[n].hz;
    require(rest, "phrases breathe", 0, id);
  }
  AudioSequencer steps;
  steps.configure(true, false, true);
  steps.select(uint8_t(GameId::Keystone), 0);
  steps.play(Sound::Footstep, 0);
  require(steps.update(0) == 330 && steps.update(8) == 180, "percussive foot contact");
  require(steps.update(24) == 0 && steps.activeEffect() == Sound::None, "short footstep gate");
  steps.update(90);
  steps.play(Sound::Hit, 90);
  steps.play(Sound::Footstep, 100);
  require(steps.activeEffect() == Sound::Hit && !steps.queued(), "footstep never delays a hit");
  steps.update(200);
  require(steps.activeEffect() == Sound::None && !steps.queued(), "no stale contact after impact");
  steps.configure(true, true, true);
  unsigned quietFrames = 0;
  for (unsigned ms = 300; ms < 1300; ++ms) {
    if ((ms - 300) % 237 == 0)
      steps.play(Sound::Footstep, ms);
    steps.update(ms);
    quietFrames += steps.activeEffect() == Sound::None;
  }
  require(quietFrames > 850, "footsteps leave room for music");
  steps.configure(true, true, false);
  steps.play(Sound::Footstep, 1500);
  require(steps.activeEffect() == Sound::None, "footsteps respect SFX mute");
  AudioSequencer footstepPreview;
  footstepPreview.configure(true, false, true);
  footstepPreview.select(uint8_t(GameId::Keystone), 0);
  FILE *pitches = std::fopen("test-results/keystone-footsteps-hz.txt", "w");
  require(pitches != nullptr, "footstep preview opens");
  for (unsigned ms = 0; ms < 1300; ++ms) {
    if (ms >= 119 && (ms - 119) % 237 == 0)
      footstepPreview.play(Sound::Footstep, ms);
    std::fprintf(pitches, "%u\n", footstepPreview.update(ms));
  }
  require(std::fclose(pitches) == 0, "footstep preview closes");
  Settings s;
  s.music = 0;
  s.sfx = 1;
  s.enhanced = 1;
  require(s.record(51, 123, 0, false) && s.record(51, 321, 3, true), "mode records");
  Settings out;
  require(decode(encode(s), out), "v3 save");
  require(out.modeBest == s.modeBest && out.music == 0 && out.enhanced == 1, "new save roundtrip");
  auto old = encode(s);
  old[4] = 2;
  old[300] = old[301] = old[302] = old[303] = 0;
  uint32_t crc = checksum(old.data(), 300);
  for (unsigned k = 0; k < 4; ++k)
    old[300 + k] = uint8_t(crc >> (8 * k));
  require(decodeStored(old.data(), 304, out), "v2 migration");
  require(out.best[51] == 321 && out.music && out.sfx && !out.enhanced && !out.modeBest[7][51],
          "legacy scores not assigned to invented category");
  auto before = encode(out), bad = encode(s);
  bad[301] = 2;
  auto badCrc = checksum(bad.data(), bad.size() - 4);
  for (unsigned k = 0; k < 4; ++k)
    bad[bad.size() - 4 + k] = uint8_t(badCrc >> (k * 8));
  require(!decode(bad, out) && encode(out) == before, "invalid migration is transactional");
  require(!decodeStored(nullptr, SaveSize, out), "null save rejected");
  Projectile projectileTest;
  projectileTest.oldX = 10;
  projectileTest.oldY = 40;
  projectileTest.x = 90;
  projectileTest.y = 40;
  require(projectileNear(projectileTest, 50, 40, 3) && !near(90, 40, 50, 40, 3),
          "actual projectile segment detects tunneling");
  Random intentRandom{42};
  FirePlan plan;
  plan.timer = 19;
  require(!plan.step(20, 40, 80, 180, 2, intentRandom, 100) && plan.cue(),
          "attack commitment starts with cue");
  auto aimX = plan.vx, aimY = plan.vy;
  for (int t = 0; t < 17; ++t) {
    require(!plan.step(20, 40, 150, 180, 2, intentRandom, 100), "cue precedes attack");
    require(plan.vx == aimX && plan.vy == aimY, "dodge cannot change committed aim");
  }
  require(plan.step(20, 40, 150, 180, 2, intentRandom, 100) && plan.timer >= 100,
          "attack recovery");
  // A wall blocks greedy pursuit; BFS takes the longer route instead.
  int route = routeStep<25>(6, 8, 5, [](int p) { return p == 7; });
  require(route != 6 && route != 7 &&
              std::abs(route % 5 - 6 % 5) + std::abs(route / 5 - 6 / 5) == 1,
          "bounded route detours around wall");
  auto &zeroGame = gameFor(GameId(0));
  GameOptions zero;
  zero.seed = 0;
  zeroGame.start(zero);
  require(zeroGame.runSeed() != 0 && zeroGame.gameplayRandomState() != 0, "zero seed normalized");
  zeroGame.cleanup();
  for (unsigned seed = 1; seed <= 50; ++seed) {
    Random r;
    r.state = seed;
    for (unsigned i = 0; i < 30; ++i) {
      int previous = int(i % 3), next = variedChoice(r, 3, previous);
      require(next >= 0 && next < 3 && next != previous, "recent-action penalty", seed);
      float aim = leadAim(30, 40, 80, 200, float(i) - 15, 3, 12);
      require(aim >= -1.25f && aim <= 1.25f, "bounded lead", seed);
    }
  }
  struct Observation {
    uint32_t score, rng;
    uint16_t stage;
    int lives;
    bool over;
  };
  // Both effects paths run real game rules. Capture observable state + gameplay RNG at every tick.
  std::array<Observation, 1200> snapshot{};
  for (unsigned id = 0; id < GameCount; ++id)
    for (unsigned seed = 1; seed <= 8; ++seed)
      for (unsigned mode = 0; mode < (id >= 50 ? 2u : 1u); ++mode) {
        for (unsigned effects = 0; effects < 2; ++effects) {
          auto &g = gameFor(GameId(id));
          GameOptions o;
          o.seed = seed;
          o.difficulty = seed % 4;
          o.effects = effects;
          o.enhanced = mode != 0;
          g.start(o);
          for (unsigned tick = 0; tick < snapshot.size(); ++tick) {
            InputFrame f;
            f.held = Fire | (tick / 80 % 2 ? Left : Right) | (tick / 130 % 2 ? Up : Down);
            f.pressed = tick % 23 == 0 ? f.held : 0;
            f.repeated = tick % 11 == 0 ? f.held : 0;
            g.handle(f);
            g.update();
            g.takeSound();
            Observation now{g.score(), g.gameplayRandomState(), g.stage(), g.lives(), g.gameOver()};
            if (!effects)
              snapshot[tick] = now;
            else {
              const auto &b = snapshot[tick];
              require(now.score == b.score && now.rng == b.rng && now.stage == b.stage &&
                          now.lives == b.lives && now.over == b.over,
                      "effects-independent gameplay", seed, id, tick);
            }
          }
          g.cleanup();
        }
      }
  std::puts("PASS audio arbitration/timeline, save migration, bounded AI, 1036800 "
            "effects-invariance ticks (Classic + Enhanced)");
}
} // namespace arcade
