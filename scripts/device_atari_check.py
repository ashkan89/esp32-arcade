"""Application-only COM9 installation with backup, section hashes and boot capture.

Requires a full current-device backup. Never writes NVS, bootloader or partitions.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import time
import serial

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('image', type=Path)
parser.add_argument('--backup', required=True, type=Path)
parser.add_argument('--port', default='COM9')
parser.add_argument('--diagnostic', action='store_true')
parser.add_argument('--network-check', action='store_true', help='Capture the private automatic update-check diagnostic through worker completion')
parser.add_argument('--previous-image', type=Path, help='Verify current flash against this image, then write only changed sections')
parser.add_argument('--log', required=True, type=Path)
args = parser.parse_args()
image = args.image.resolve()
data = image.read_bytes()
backup = args.backup.read_bytes()
assert len(backup) == 0x400000, 'Expected full 4MB backup'
assert data[0] == 0xe9 and data[3] >> 4 == 2 and len(data) <= 0x1f0000
version = (root / 'VERSION').read_text().strip()
assert ('SKY_ARCADE_WROOM_N4|' + version).encode() + b'\0' in data
parts = {}
for offset in range(0x8000, 0x8c00, 32):
    magic, typ, sub, address, size, label, flags = struct.unpack('<HBBII16sI', backup[offset:offset+32])
    if magic != 0x50aa:
        break
    parts[label.split(b'\0')[0].decode()] = (address, size)
assert parts['app0'] == (0x10000, 0x1f0000) and parts['app1'] == (0x200000, 0x1f0000)
assert parts['nvs'] == (0x9000, 0x5000) and parts['otadata'] == (0xe000, 0x2000)
# This installation path preserves OTA selection and only replaces app0.
ota_seq, ota_state = struct.unpack_from('<I20xI', backup, 0xe000)
assert ota_seq == 1 and ota_state == 2, 'Expected the verified app0 selection on this device'
tool = Path.home() / '.platformio/packages/tool-esptoolpy/esptool.py'
base = [sys.executable, str(tool), '--chip', 'esp32', '--port', args.port, '--baud', '115200']
identity = subprocess.check_output(base + ['flash_id'], text=True)
assert 'd4:e9:f4:8b:84:08' in identity.lower() and 'Detected flash size: 4MB' in identity, identity
current = root / '.cache/com9-atari-current-partitions.bin'
subprocess.run(base + ['read_flash', '0x8000', '0x1000', str(current)], check=True, stdout=subprocess.DEVNULL)
assert current.read_bytes() == backup[0x8000:0x9000], 'Partition layout changed since backup'
print('Verified device, 4 MB image and partitions; backup SHA256', hashlib.sha256(backup).hexdigest(), flush=True)
previous = None
if args.previous_image:
    result = subprocess.run(base + ['verify_flash', '0x10000', str(args.previous_image.resolve())],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    assert result.returncode == 0, 'Previous-image verification failed: ' + result.stdout
    previous = args.previous_image.read_bytes()
    print('PASS current application matches previous image; unchanged sections can be retained', flush=True)
args.log.parent.mkdir(exist_ok=True, parents=True)
with args.log.with_suffix('.flash.log').open('w') as log:
    for index, offset in enumerate(range(0, len(data), 65536)):
        if previous is not None and data[offset:offset+65536] == previous[offset:offset+65536]:
            continue
        section = root / '.cache/com9-atari-section.bin'
        section.write_bytes(data[offset:offset+65536])
        address = 0x10000 + offset
        assert 0x10000 <= address and address + section.stat().st_size <= 0x200000
        for attempt in range(3):
            result = subprocess.run(base + ['write_flash', '--flash_size', 'keep', hex(address), str(section)],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            log.write(result.stdout); log.flush()
            if result.returncode == 0 and 'Hash of data verified.' in result.stdout:
                break
            print(f'Retry section {index}, attempt {attempt+1}', flush=True)
            time.sleep(.5)
        else:
            raise RuntimeError('Section upload failed; restore the application from backup before disconnecting')
        print(f'PASS section {index+1}/{(len(data)+65535)//65536} {address:#x}', flush=True)
    result = subprocess.run(base + ['verify_flash', '0x10000', str(image)],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    log.write(result.stdout); log.flush()
    assert result.returncode == 0, result.stdout
print('PASS whole application verification SHA256', hashlib.sha256(data).hexdigest(), flush=True)
port = serial.Serial()
port.port = args.port; port.baudrate = 115200; port.timeout = .2
port.dtr = False; port.rts = False; port.open()
port.rts = True; time.sleep(.1); port.rts = False
deadline = time.monotonic() + (180 if args.network_check else 90 if args.diagnostic else 28)
output = bytearray()
pending = bytearray()
with args.log.open('wb') as log:
    while time.monotonic() < deadline:
        chunk = port.read(port.in_waiting or 1)
        if not chunk:
            continue
        output.extend(chunk); pending.extend(chunk); log.write(chunk); log.flush()
        while b'\n' in pending:
            line, _, pending = pending.partition(b'\n')
            print(line.decode('ascii', errors='replace').encode('ascii', errors='replace').decode('ascii'), flush=True)
        if b'Boot health checks passed; firmware confirmed' in output and (not args.network_check or b'[network] Done:' in output):
            break
port.close()
text = output.decode('ascii', errors='replace')
assert 'Boot health checks passed; firmware confirmed' in text
assert 'Settings and records loaded' in text
assert not any(t in text for t in ['Guru Meditation', 'Brownout detector', 'abort() was called',
                                  'watchdog got triggered', 'ART_CHECK FORMAT FAILED'])
if args.diagnostic:
    assert 'ART_CHECK game=50 ' in text and 'ART_CHECK game=51 ' in text
    assert 'ART_CHECK COMPLETE' in text
    assert text.count('compact=1 full=1 rgb=16') == 10
else:
    assert 'ART_CHECK BEGIN' not in text
if args.network_check:
    assert 'UPDATE_DIAGNOSTIC automatic check only' in text and '[network] Done:' in text
    print('PASS network diagnostic captured; inspect status for check result', flush=True)
else:
    print('PASS on-device diagnostic and boot health' if args.diagnostic else 'PASS normal firmware and boot health', flush=True)
