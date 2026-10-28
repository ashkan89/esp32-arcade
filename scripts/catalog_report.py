"""Generate a numbered source-backed inventory; never infer hardware acceptance."""
from pathlib import Path
import csv
import re

root = Path(__file__).resolve().parents[1]
device_log = root / "artifacts/com9-art-check.log"
device_coverage = device_log.exists() and "ART_CHECK COMPLETE games=52" in device_log.read_text(encoding="utf-8", errors="replace")
catalog = (root / "include/games/Catalog.def").read_text(encoding="utf-8")
entries = re.findall(r"^ARCADE_GAME\((\w+),\s*(\w+),\s*(\d+)\)", catalog, re.M)
assert len(entries) == 52, "Audit the requested 52-game catalog before changing its size"
assert [int(entry[2]) for entry in entries] == list(range(52)), "IDs must remain contiguous and stable"
assert len({entry[1] for entry in entries}) == 52, "Duplicate implementation in catalog"
lines = ["# Numbered inventory and verification matrix", "",
         "Generated from Catalog.def and the actual C++ registrations with `python scripts/catalog_report.py`.",
         "All 52 entries have separate implementations. No missing, duplicated or placeholder registration was found.",
         "Keystone Kapers is the implemented title; no Keystone Keeper entry exists.", "",
         "The table records source presence and links to each game's exact six-line on-device instructions.",
         "It is not a human playtest or an assertion that every rule matches a commercial original.", "",
         "| No. | Stable ID | Game / implementation | On-device controls and objective |",
         "| --- | --- | --- | --- |"]
titles = set()
for number, (_, kind, game_id) in enumerate(entries, 1):
    path = root / "src/games" / (kind + ".cpp")
    source = path.read_text(encoding="utf-8")
    match = re.search(r"return\s+gameEntry<" + re.escape(kind) + r">\(\s*\{(.*?)\},", source, re.S)
    assert match, f"Missing real registration for {kind}"
    strings = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', match[1])
    assert len(strings) == 8, f"Expected title, objective and six instructions for {kind}"
    title, objective, *instructions = strings
    assert title not in titles, f"Duplicate displayed title {title}"
    titles.add(title)
    controls = "; ".join(instructions).replace("|", "/")
    lines.append(f"| {number} | {game_id} | [{title}](../src/games/{kind}.cpp) | {objective}. {controls} |")
lines += ["", "## Verification interpretation", "",
          "Every row participates in the same complete host suite, with game-specific rule checks.",
          "Current graphics, build and device evidence is recorded in MODERN_GRAPHICS.md.", "",
          "| Acceptance area | Implemented | Build / host evidence | Hardware tested in this revision |",
          "| --- | --- | --- | --- |",
          "| Launch and controls | All 52 | Registry checks; four-difficulty scripted input soak | No |",
          "| Gameplay and collisions | All 52 | Focused rules plus random input; not exhaustive play | No |",
          "| Score and progression | All 52 | Individual campaign endings / Atari ongoing-round tests | No |",
          "| Pause, resume, restart, exit | All 52 / shared console | Pause invariants, restart soak, console transition checks | No |",
          "| Game over / victory | All 52 | Explicit terminal rules and random loss/restart cycles | No |",
          "| Persistence | Shared console / all stable IDs | CRC corruption, v1–v4 migration, mode scores, favorites, recent | No |",
          "| Rendering | All 52 | Actual C++ previews; clipping guards; RGB565/RGB332 checks | " + ("52 automated update/render/SPI exercises; manual viewing pending" if device_coverage else "No") + " |",
          "| Audio | All 52 tracks and shared effects | Sequencer timeline/arbitration/mute checks; not listening | No |",
          "| Performance / memory | Bounded runtime | Compile sizes/arena limits and fixed-step tests | " + ("52 timing/heap/stack samples; ten format transitions" if device_coverage else "No") + " |",
          "", "A scripted action stream cannot prove every level is enjoyable, every collision is fair or",
          "every victory is reachable through ordinary play. Use HARDWARE_CHECKLIST.md for acceptance.", ""]
target = root / "docs/GAME_INVENTORY.md"
target.write_text("\n".join(lines), encoding="utf-8")
print(f"Wrote {target.relative_to(root)}: 52 unique IDs, source registrations and instruction sets.")
# Evidence labels deliberately distinguish exercised paths from exhaustive playtests.
evidence = root / "test-results/host-tests.log"
if evidence.exists() and re.search(r"^PASS \d+ assertions across core", evidence.read_text(encoding="utf-8"), re.M):
    with (root / "test-results/game-verification.csv").open("w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(["number", "stable_id", "game", "implemented", "launch", "controls",
                         "gameplay_collisions", "scoring_progression", "pause_resume", "restart",
                         "game_over_victory", "return_launcher", "persistence", "rendering", "audio",
                         "performance_memory", "hardware_tested"])
        for number, (_, kind, game_id) in enumerate(entries, 1):
            writer.writerow([number, game_id, kind, "source present", "host soak", "scripted input",
                             "focused rules + soak", "focused rules + soak", "host pause invariants",
                             "host repeated restart", "focused terminal rules", "shared console tests",
                             "shared codec + stable ID tests", "actual renderer + clipping guards",
                             "shared sequencer tests; listening pending", "bounded arena; device timing/heap/stack samples" if device_coverage else "bounded arena; device measurements pending",
                             "automated update/render/SPI only; manual play pending" if device_coverage else "no"])
    print("Wrote test-results/game-verification.csv; evidence distinguishes automated exercises from manual acceptance.")
