# Flash write protection

Frame history was a volatile PSRAM screenshot ring, with no flash writes. It and its system-menu
entry are now removed from all targets. Cargo Shift's separate in-RAM undo history remains.

## Application persistence

- Settings, favorites, last game and records share the existing 2,356-byte `skyarcade/state` NVS
  blob. Changes wait for two quiet seconds and a screen outside gameplay/attract. Each write
  attempt starts a 60-second cooldown, including failures. New edits replace the pending RAM
  state. The next eligible attempt saves the newest state, and failures stay pending.
- A RAM snapshot of loaded/successfully saved bytes suppresses duplicate dirty notifications
  and changes reverted before saving. Default settings are used without writing a state blob
  on boot. Existing v1/v2 or historical-namespace data is still migrated once when safe; its
  original data remains available for firmware rollback.
- Wi-Fi setup stores one JSON string at `arcade-net/config`. Submitting identical saved JSON
  skips writing. Changed configurations save immediately on explicit submission, with attempts
  limited to once per minute across setup sessions. A request during the cooldown receives
  HTTP 429; failures receive HTTP 500. Rejected/failed submissions do not replace active settings.
- Timers/counters are RAM-only and reset on reboot. These limits apply independently to each
  application save path; they do not cap every flash operation performed by the SDK.

Pending settings can be lost if power is cut during the quiet period or cooldown, or before a
safe screen is reached. The cooldown does not force writes during gameplay. Leave the console
on a menu for at least 60 seconds after the final edit, and watch for `Settings and records saved`
when checking persistence on a board. A repeated identical edit does not need another save.

## Other flash operations

No EEPROM API, filesystem logging or per-frame flash storage is used by this project.
`WiFi.persistent(false)` is set before Wi-Fi initialization, so the pinned Arduino core uses
`WIFI_STORAGE_RAM` for Wi-Fi driver configuration. First-time namespace creation, SDK radio
calibration, firmware uploads, requested OTA installs/rollback and boot confirmation can still
write flash. An already-valid OTA image returns from confirmation without changing OTA data.
Release discovery rejects versions that are not newer before permitting installation.

NVS already provides flash wear levelling; fewer application writes reduce pressure on it.
The existing NVS partition, keys and serialized save format remain compatible. No lifetime
guarantee is implied without the flash chip's endurance specification and actual write workload.
See [Espressif's NVS documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/storage/nvs_flash.html).

The WROVER linker safeguard in `scripts/defer_himem.py` still suppresses the SDK's premature
himem constructor to preserve the prior boot-loop fix. It no longer initializes or allocates a
frame-history buffer. Its post-link check accepts a removed himem symbol and verifies that any
retained initialization symbol is absent from the constructor table.

## Verification

Host tests count writer calls for no-change boot, unsafe screens, debounce, duplicate marks,
reverted edits, cooldown, failed retries, newest-state saves, migration and millisecond rollover.
Utility-menu tests check wrapping through the remaining five entries. Build all four environments
with `pio run -e release -e debug -e wroom-4mb -e wroom-16mb` and run
`python scripts/test_host.py`. Physical reboot, provisioning, power-loss and flash endurance
tests remain required; host tests cannot measure physical erase counts.

Local verification on 2026-10-12: all four firmware environments passed with the final source,
including both WROVER constructor checks. The host suite passed 2,282,314 reported assertions,
the persistence writer-call checks and 2,080,000 lifecycle ticks. The actual C++ menu preview
was inspected with five utility entries and no frame-history entry. Logs are in
`test-results/flash-wear-final-builds.log` and `test-results/host-tests.log`; the preview is
`artifacts/menu-preview.png`. No firmware upload or physical endurance test was performed.
