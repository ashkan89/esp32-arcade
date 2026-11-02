"""Import the specified local ROM and pinned RiverRaid tables; no runtime downloads.

Usage: python scripts/import_atari_sources.py --river-source .cache/RiverRaid-source
ROM bytes and disassembly remain ignored local build inputs.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--river-source', type=Path)
parser.add_argument('--disassemble', action='store_true', help='Write reachable 6502 code and cartridge analysis to test-results (requires py65)')
args = parser.parse_args()
rom = root / 'roms/Keystone Kapers (USA).a26'
if not rom.is_file():
    raise SystemExit('Missing local build input: roms/Keystone Kapers (USA).a26 (see docs/ATARI_SOURCE_PORTS.md)')
data = rom.read_bytes()
if len(data) != 4096 or data[-4:] != bytes.fromhex('00f000f0'):
    raise SystemExit('Expected the supplied 4K Keystone Kapers USA cartridge with F000 reset vector')
digest = hashlib.sha256(data).hexdigest()
if digest != '10fc464822fcc94d5573a772a9a4fdaf17c4d08a2e73015ab010f08ed124b02d':
    raise SystemExit('This runtime adapter expects the supplied Keystone Kapers USA ROM hash; inspect other revisions before use')
generated = root / 'include/generated'
generated.mkdir(exist_ok=True)
rom_header = (
    '// Local user-supplied cartridge; do not distribute without rights.\n#pragma once\n'
    'namespace arcade { static constexpr unsigned char KeystoneRom[4096] = {\n' +
    ',\n'.join(','.join(f'0x{b:02x}' for b in data[i:i+32]) for i in range(0, len(data), 32)) +
    '\n}; }\n')
header_path = generated / 'KeystoneRom.h'
if not header_path.exists() or header_path.read_text(encoding='utf-8') != rom_header:
    header_path.write_text(rom_header, encoding='utf-8')
# The Atari's 6507 has no 6510 port pins. A 32-bit pin bus contains all address,
# data and 6502 control pins and avoids emulating an unused high word on ESP32.
# Keep upstream pristine; generate and clearly mark this narrow adaptation.
cpu_source = (root / 'include/third_party/m6502.h').read_text(encoding='utf-8')
cpu = cpu_source.replace('uint64_t', 'uint32_t')
cpu, changes = __import__('re').subn(r'#define M6510_SET_PORT\(p,d\).*',
                                  '#define M6510_SET_PORT(p,d) ((void)(d))', cpu)
assert changes == 1, 'Upstream CPU port macro changed; review the 6507 adaptation'
cpu = ('// GENERATED MODIFIED SOURCE: 32-bit pin bus for Atari 6507 only; 6510 port output disabled.\n'
       '// Generated from the unmodified pinned m6502.h by scripts/import_atari_sources.py.\n' + cpu)
cpu_path = generated / 'KeystoneCpu.h'
if not cpu_path.exists() or cpu_path.read_text(encoding='utf-8') != cpu:
    cpu_path.write_text(cpu, encoding='utf-8')
print('Keystone ROM SHA256:', digest)
if args.disassemble:
    from py65.devices.mpu6502 import MPU
    from py65.disassembler import Disassembler
    memory = MPU()
    memory.memory[0xf000:] = data
    dis = Disassembler(memory)
    todo, visited, listing = [0xf000], set(), {}
    branches = {'BCC', 'BCS', 'BEQ', 'BMI', 'BNE', 'BPL', 'BVC', 'BVS'}
    while todo:
        address = todo.pop()
        while 0xf000 <= address < 0xfffa and address not in visited:
            visited.add(address)
            length, instruction = dis.instruction_at(address)
            listing[address] = instruction
            opcode = instruction.split()[0]
            if opcode in branches or opcode in {'JSR', 'JMP'}:
                if instruction[4:5] == '$':
                    todo.append(int(instruction[5:], 16))
                if opcode == 'JMP':
                    break
            if opcode in {'RTS', 'RTI', 'BRK', '???'}:
                break
            address += length
    out = root / 'test-results'
    out.mkdir(exist_ok=True)
    (out / 'keystone-reachable.asm').write_text(
        '; Local analysis of user-supplied ROM SHA256 ' + digest + '\n' +
        '\n'.join(f'{a:04X}: {listing[a]}' for a in sorted(listing)) + '\n')
    (out / 'keystone-analysis.json').write_text(json.dumps({
        'sha256': digest, 'bytes': len(data), 'mapper': '4K, no bankswitching',
        'reset': 'F000', 'reachable_instructions': len(listing),
        'observed_ram': {'88': 'active-play bit 7', '96': 'reserve officers',
                         '9A:9B:9C': 'packed BCD score (adders FDB6/FDB8)', '9D': 'packed BCD timer (init FA37)'}
    }, indent=2) + '\n')
if args.river_source:
    source = args.river_source
    def table(file):
        tree = ast.parse((source / file).read_text(encoding='utf-8-sig'))
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign) and any(isinstance(t, ast.Attribute) and t.attr == 'mp' for t in node.targets):
                return ast.literal_eval(node.value)
        raise ValueError(file)
    colors = ast.literal_eval(ast.parse((source / 'cores.py').read_text()).body[-1].value)
    def rgb565(n):
        return ((n >> 19) & 31) << 11 | ((n >> 10) & 63) << 5 | ((n >> 3) & 31)
    text = '// Imported from athaless/RiverRaid d6dcebe7e62e41d0a0566cb57694edd34406b7d8.\n#pragma once\n#include <cstdint>\nnamespace arcade { namespace river_source {\n'
    text += 'static constexpr uint16_t palette[] = {' + ','.join(hex(rgb565(c)) for c in colors) + '};\n'
    for name, filename in [('banks', 'place.py'), ('islands', 'ilhas.py')]:
        rows = table(filename)
        for i, row in enumerate(rows):
            text += f'static constexpr uint8_t {name}{i}[] = {{' + ','.join(map(str, row)) + '};\n'
        text += f'static constexpr const uint8_t *{name}[] = {{' + ','.join(f'{name}{i}' for i in range(len(rows))) + '};\n'
        text += f'static constexpr int {name}Length[] = {{' + ','.join(str(len(r)) for r in rows) + '};\n'
    rows = table('obj.py')
    for i, sprite in enumerate(rows):
        text += f'static constexpr uint8_t sprite{i}[] = {{' + ','.join(str(v) for r in sprite for v in r) + '};\n'
    text += 'struct Sprite { const uint8_t *data; int w, h; };\nstatic constexpr Sprite sprites[] = {\n'
    text += ',\n'.join('{sprite%d,%d,%d}' % (i, len(s[0]), len(s)) for i, s in enumerate(rows)) + '};\n}}\n'
    (root / 'include/assets/RiverSource.h').write_text(text, encoding='utf-8')
    print('Imported bank, island, palette and sprite tables')
