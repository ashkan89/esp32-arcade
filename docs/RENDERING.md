# Rendering and frame pacing

The October 12, 2026 investigation found CPU and pacing overhead in addition to the panel's
bandwidth limit. The earlier board capture in VERIFICATION.md measured about 74.6 ms for
full-frame presentation, so the existing 30 FPS setting was not evidence of achieved 30 FPS.

## Findings and changes

- Simulation previously used 16,667 us while rendering used 16,666 us at 60 FPS or 33,333 us
  at 30 FPS. Separate phases and a timestamp sampled before audio/input work could render a
  repeated simulation state. Both clocks now accumulate fractional microseconds from the same
  sample and reset together. Exactly 60 simulation ticks fit one second; 30 FPS samples every
  second tick. Long stalls retain bounded simulation catch-up and skip stale render deadlines.
- Dense snapshot frames previously recalculated a bit-at-a-time CRC over all RGB565 pixels,
  even when exact comparison already established changes. This CRC work is removed. Changes
  remain confined to affected bands rather than crossing a tile-count threshold that triggers
  a full-screen transfer. Adjacent tiles form runs, and the planner chooses separated runs or a
  trimmed band using boot-measured window overhead and configured SPI bandwidth.
- Filled rectangles previously called the clipped pixel setter for every pixel. Row fills
  now use bulk writes. RGB565 staging uses row copies, while RGB332 expansion uses a 256-entry
  lookup table. WROOM checks native bands before converting them, and its CRC uses a small
  nibble table compatible with existing saves and update checksums.
- One expensive render previously disabled 60 FPS for the rest of the boot. The new policy
  checks 30-frame windows and falls back when at least eight exceed 14,667 us, reserving about
  2 ms for other work. Screen transitions retry the requested rate. This is a workload policy,
  not a guarantee of physical refresh rate.
- Frame-history capture and playback have been removed; no banked screenshot allocation or
  capture work remains in the rendering loop.
- Near a render deadline the loop polls in short intervals instead of always sleeping a full
  RTOS tick. Away from deadlines it continues yielding normally.

## Physical limit and verification

At 176 × 220 × 16 bits and 20 MHz SPI, the pixel payload alone takes **30.976 ms** for a
full screen. Controller commands, staging, drawing, simulation and scheduling add to that.
Full-screen 60 FPS is impossible on this transfer configuration, and full-screen 30 FPS has
only about 2.36 ms of theoretical headroom. Sparse updates are essential. No SPI overclock or
panel-specific DMA change was introduced without hardware validation.

Host timing regressions cover ten simulated minutes at each rate with polling jitter,
variable transfer costs, timer wraparound, exact tick counts, stalls, and sustained-load
fallback. Pixel tests cover dirty erasure, separated/merged runs, RGB332 palette values,
clipping, packed staging, inversion, and the standard CRC check vector. Existing catalog
gameplay and lifecycle tests remain part of validation. These checks cannot measure ESP32
transfer speed, panel scanout or tearing.

Validation passed 2,282,057 assertions in the full host suite, including 2,080,000 lifecycle
ticks and 1,036,800 effects-invariance ticks. A final isolated run of the current rendering
regressions passed 54,950 checks, including all 256 RGB332 palette entries. Logs are retained
in ignored `test-results/host-tests.log` and the local rendering test cache.
Final firmware builds passed for release, debug, WROOM 4 MB and WROOM 16 MB; the log is
`test-results/rendering-builds-final.log`. Local application/factory packages were refreshed
in `dist/` for all three release boards. No device was flashed and no release was published.

A desktop GCC `-O2` microbenchmark compared HEAD's original Canvas/Core with these changes
over 1,000 repetitions of a synthetic filled scene. RGB565 drawing averaged 340.46 → 78.60 us;
RGB332 drawing averaged 409.30 → 29.04 us. Full-buffer CRC averaged 1,184.51 → 508.94 us and
602.26 → 245.27 us respectively. Both modes produced identical final pixel checksums.
Concurrent build load can affect these figures; they demonstrate CPU savings on the host,
not ESP32 FPS or the planner's physical SPI cost. The local log is
`test-results/rendering-cpu-benchmark.log`.

For device acceptance, run the debug firmware and select each FPS request. Exercise dense
and sparse scenes, pause/resume, restart and inversion on each supported board. Record the
`DISPLAY achieved=... recentWorst=...`, `PACING target=... missedDeadlines=...`, `COST`, and
`FRAME` serial lines. SYSTEM also shows measured presentation FPS. The measured FPS counts
completed presentations, including unchanged frames; inspect motion and the panel itself
to establish visible smoothness. Compare sustained gameplay windows rather than boot/menu
transitions. The optimized firmware has not yet been measured on the physical board.
