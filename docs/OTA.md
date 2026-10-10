# GitHub A/B OTA

Default repository: `ashkan89/esp32-arcade`, editable through the on-device Wi-Fi setup portal.
Only public repositories and newer stable `major.minor.patch` releases are accepted. Discovery
uses GitHub's `/repos/{owner}/{repo}/releases/latest` REST endpoint. Draft/prerelease releases,
equal/older versions, missing assets, wrong boards and oversized images are rejected.

Required release assets:

- `arcade-esp32-n16r8.bin`: application image, **not** a merged flash image.
- `arcade-release.json`: schema 1, board `esp32-wrover-n16r8-ili9225`, semantic version,
  asset filename, exact byte size, and 64-digit SHA-256.

`python scripts/package_release.py` creates these assets in `dist/`, plus a merged
`arcade-factory-esp32-n16r8.bin` for initial USB flashing at address `0x0`, and `SHA256SUMS.txt`.
The factory image includes bootloader, partition table and application; it contains blank NVS
and OTA-data regions, so flashing it resets saved settings. A normal PlatformIO upload is the
preferred first-install path. Subsequent OTA updates preserve settings and partition layout.

The firmware uses the SDK's full certificate bundle with hostname verification; it never calls
`setInsecure`. Time must synchronize through NTP before HTTPS. Redirects are bounded and allowed
only to GitHub and its known release-asset hosts, over HTTPS. Downloads have read/overall deadlines.
JSON is bounded in PSRAM. The binary is streamed through an internal 4 KB buffer and hashed.
Checks include manifest/release version agreement, release asset size, target chip, an exact
compiled board/version identity marker, SHA-256, and `esp_ota_end()` image validation. TLS plus
the manifest hash trusts the GitHub repository owner; this is not secure boot or signed firmware.

| Partition | Address | Size | Purpose |
| --- | --- | --- | --- |
| NVS | `0x9000` | `0x5000` | Settings and Wi-Fi configuration |
| otadata | `0xe000` | `0x2000` | Two redundant boot-selection sectors |
| app0 / ota_0 | `0x10000` | `0x300000` | A application slot |
| app1 / ota_1 | `0x310000` | `0x300000` | B application slot |

Until verification completes, the running image remains the selected boot image. Only then
does `esp_ota_set_boot_partition()` select the new image as `NEW`. On its next boot, the bootloader
sets `PENDING_VERIFY`. The application overrides Arduino's `verifyRollbackLater()` hook to avoid
the core's early automatic acceptance. Startup validates detected 16 MB flash / 8 MB PSRAM,
display allocation/dimensions, NVS availability, and at least eight seconds, 20 rendered frames,
and 60 main-loop updates before marking the image valid. Reset/crash before acceptance triggers
bootloader rollback. A detected startup failure explicitly rejects the pending image.

The pinned SDK configuration enables `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`. Install the bundled
bootloader via the initial serial upload; adding a flag to an old bootloader cannot enable rollback.
Explicit **Boot Previous Firmware** accepts only a partition previously marked healthy.
With a fresh single-image install, there is no previous image until the first successful OTA.
Two slots preserve one previous healthy firmware; a second update overwrites that older slot.

## Publish the next version

Increase `VERSION`, update `docs/RELEASE_NOTES.md`, and commit. Create a tag matching the version
exactly, such as `v2.0.1`, then push the commit and tag to GitHub. `.github/workflows/release.yml`
builds release/debug, runs host tests, packages checked assets, and creates the release.
Manual workflow dispatch validates builds without publishing a release. Never edit or replace
firmware assets in an already published version; publish a higher version instead.

Official references: [ESP-IDF 4.4.7 OTA and rollback](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/ota.html),
[GitHub release API](https://docs.github.com/en/rest/releases/releases).
