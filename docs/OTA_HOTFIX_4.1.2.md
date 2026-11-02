# Updater hotfix 4.1.2

The COM9 regression test reproduced an asset-download failure: GitHub discovery returned
HTTP 200, the release URL redirected with HTTP 302, and the release CDN returned HTTP 618.
Refreshing only the browser download URL was insufficient for the firmware asset.
This affected checks that needed to fetch a newer release's manifest, as well as installation.

The updater now prefers the release asset's REST `url`, requesting file bytes with
`Accept: application/octet-stream`, as supported by [GitHub's asset download API](https://docs.github.com/en/rest/releases/assets#get-a-release-asset).
It adds a unique nonce to the original GitHub asset request and sends no-cache headers.
Signed CDN redirect URLs are never modified. HTTP 618 and transient network/server failures
receive one retry through the original endpoint. Rate limits and permanent access failures
receive specific messages instead of immediate repeated requests.

TLS certificate/hostname verification, allowed HTTPS hosts, redirect limits, target board,
version agreement, image size, compiled identity and SHA-256 requirements remain enforced.
An up-to-date check is remembered: Install then reports **NO NEWER FIRMWARE TO INSTALL**
without starting Wi-Fi again. Exactly-at-limit JSON bodies are accepted; oversized,
truncated or malformed bodies still fail.

## Device result

**Normal 4.1.2 is installed on COM9 via USB**, with whole-image verification and successful
boot-health confirmation. The image is 1,626,160 bytes; SHA-256:
`2a456b4502dbf3e0eead86b9e8ffdd5f142c116ce9c9bc3d346b201ef9f0b608`.
Normal startup has no automatic diagnostic check or download.

The read-only regression diagnostic used archived release v4.0.1, the release that
previously returned HTTP 618. Discovery, the manifest and the complete 1,575,104-byte
firmware download succeeded on the ESP32. The firmware's board/header/version identity,
size and SHA-256 matched. The worker finished with 6,220 bytes of stack headroom and
36,032 bytes minimum internal heap observed during the test.

The diagnostic intentionally did not call OTA begin/write/end or boot selection. It
therefore verified download and validation, not an OTA installation/reboot. The running
firmware was replaced by the normal USB build after the test. A normal device only installs
a published version newer than its installed version.

Bootloader, partition table and OTA selection read back identical to the pre-fix backup.
The saved-state blob (settings, scores, favorites and history) and Wi-Fi/repository
configuration also compared identical, with valid saved-state CRC. Raw NVS storage
bookkeeping changed during network activity; the complete user-state blob and network
configuration values compared equal.

The host suite passed 2,282,611 regular assertions plus the existing focused game checks,
including the new HTTP-618/retry policy, API-vs-signed-URL classification, owning asset URL
parsing, exact JSON limit, chunked completion and oversized-body regressions.
All four normal configurations (`release`, `debug`, `wroom-4mb`, `wroom-16mb`)
built successfully.

Evidence retained locally:

- `artifacts/com9-update-fixed-download.log`: reproduced HTTP 618.
- `artifacts/com9-update-fresh-links.log`: browser-link refresh still failed for the binary.
- `artifacts/com9-update-asset-api.log`: manifest and complete firmware verification passed.
- `artifacts/com9-update-hotfix-normal.log` and its flash log: installed normal 4.1.2.
- `test-results/update-host-tests.log`, `update-final-images.json`, board build logs,
  `update-protected-verification.json` and `update-userdata-verification.json`.

`pio run -e update-check` builds the private read-only diagnostic, which automatically
checks archived v4.0.1 after a healthy boot. It downloads/verifies bytes but never writes
or selects an OTA image. Replace it with a normal board build after use.
