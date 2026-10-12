#pragma once
#include "Core.h"
namespace arcade {
// Runtime limits; no counters or timestamps are themselves saved to flash.
class WriteCooldown {
  uint32_t lastAttempt = 0;
  bool attempted = false;

public:
  static constexpr uint32_t IntervalMs = 60000;
  bool eligible(uint32_t now) const {
    return !attempted || elapsed(now, lastAttempt) >= IntervalMs;
  }
  void noteAttempt(uint32_t now) {
    lastAttempt = now;
    attempted = true;
  }
};
enum class SaveResult { Deferred, Unchanged, Saved, Failed };
template <class Bytes> class DeferredSave {
  Bytes persisted{};
  WriteCooldown cooldown;
  uint32_t changed = 0;
  bool hasPersisted = false, dirty = false;

public:
  void remember(const Bytes &bytes) {
    persisted = bytes;
    hasPersisted = true;
    dirty = false;
  }
  void mark(uint32_t now) {
    dirty = true;
    changed = now;
  }
  bool due(uint32_t now, bool safe) const {
    return dirty && safe && elapsed(now, changed) >= 2000 && cooldown.eligible(now);
  }
  template <class Writer> SaveResult update(const Bytes &bytes, uint32_t now, bool safe,
                                           Writer write) {
    if (!due(now, safe))
      return SaveResult::Deferred;
    if (hasPersisted && bytes == persisted) {
      dirty = false;
      return SaveResult::Unchanged;
    }
    cooldown.noteAttempt(now);
    if (!write(bytes)) {
      // A failed commit can leave the physical state uncertain. Do not suppress
      // a later compensating save even if the user reverts to the old snapshot.
      hasPersisted = false;
      return SaveResult::Failed;
    }
    remember(bytes);
    return SaveResult::Saved;
  }
};
} // namespace arcade
