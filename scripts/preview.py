"""Pack the actual C++ renderer's PPM output into a PNG contact sheet, without dependencies."""
from pathlib import Path
import argparse
import struct
import zlib
import re

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--menus", action="store_true", help="Export only console screens for UI review")
parser.add_argument("--atari", action="store_true", help="Export the two Atari game renderers")
parser.add_argument("--atari-states", action="store_true", help="Export Atari gameplay regression scenes")
parser.add_argument("--games", help="Catalog ID range, start:end (end exclusive), e.g. 0:16")
parser.add_argument("--rgb332", action="store_true", help="Preview exact WROOM RGB332 color quantization")
args = parser.parse_args()
panels = sorted((root / "test-results").glob("preview_*.ppm"))
if args.games:
    begin, end = map(int, args.games.split(':'))
    assert 0 <= begin < end <= 52
    panels = [p for p in panels if begin < int(p.stem.split('_')[1]) <= end]
if args.atari_states:
    panels = sorted((root / "test-results").glob("atari_*.ppm"))
if args.menus:
    game_count = len(re.findall(r"^ARCADE_GAME\(", (root / "include/games/Catalog.def").read_text(), re.M))
    panels = [p for p in panels if int(p.stem.split("_")[1]) == 0 or int(p.stem.split("_")[1]) > game_count]
if args.atari:
    panels = [p for p in panels if int(p.stem.split("_")[1]) in (51, 52)]
if len(panels) < (2 if args.atari else 3):
    raise SystemExit("Run python scripts/test_host.py first.")
cols, scale = (4, 2) if args.games or args.atari_states else (2, 3) if args.atari else (5, 2)
rows = (len(panels) + cols - 1) // cols
width, height = (176 + 8) * cols * scale, (220 + 8) * rows * scale
pixels = bytearray(bytes((18, 21, 32)) * width * height)
for index, path in enumerate(panels):
    header, dimensions, limit, data = path.read_bytes().split(b"\n", 3)
    assert header == b"P6" and dimensions == b"176 220" and limit == b"255"
    for y in range(220):
        for x in range(176):
            color = data[(y * 176 + x) * 3:(y * 176 + x + 1) * 3]
            if args.rgb332:
                r, g, b = color
                color = bytes((((r >> 5) * 31 // 7) * 255 // 31,
                               ((g >> 5) * 63 // 7) * 255 // 63,
                               ((b >> 6) * 31 // 3) * 255 // 31))
            xx, yy = (index % cols * 184 + x + 4) * scale, (index // cols * 228 + y + 4) * scale
            for dy in range(scale):
                for dx in range(scale):
                    offset = ((yy + dy) * width + xx + dx) * 3
                    pixels[offset:offset + 3] = color

def chunk(kind, payload):
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

raw = b"".join(b"\0" + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
stem = f'games-{begin:02d}-{end-1:02d}' if args.games else 'atari-states' if args.atari_states else 'atari-preview' if args.atari else 'menu-preview' if args.menus else 'console-preview'
artifact = root / 'artifacts' / (stem + ('-rgb332' if args.rgb332 else '') + '.png')
artifact.parent.mkdir(exist_ok=True)
artifact.write_bytes(png)
print(f"Wrote {artifact.relative_to(root)} from the real game/menu renderer.")
