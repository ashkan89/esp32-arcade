"""Package the application for GitHub OTA and a separate initial USB-flash image."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import struct

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--tag',default=None)
parser.add_argument('--env', choices=['release', 'wroom-4mb', 'wroom-16mb'], default='release')
args=parser.parse_args()
version=(root/'VERSION').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+',version):raise SystemExit('Invalid VERSION')
if args.tag and args.tag!='v'+version:raise SystemExit('Release tag must match VERSION exactly')
targets = {
    'release': ('n16r8', 'esp32-wrover-n16r8-ili9225', 'SKY_ARCADE_N16R8|', 'arcade-release.json', 16, 0x300000),
    'wroom-4mb': ('wroom-n4', 'esp32-wroom-n4-ili9225', 'SKY_ARCADE_WROOM_N4|', 'arcade-release-wroom-n4.json', 4, 0x1f0000),
    'wroom-16mb': ('wroom-n16', 'esp32-wroom-n16-ili9225', 'SKY_ARCADE_WROOM_N16|', 'arcade-release-wroom-n16.json', 16, 0x300000),
}
suffix, board, identity, manifest_name, flash_mb, slot_size = targets[args.env]
build=root/'.pio/build'/args.env
image=(build/'firmware.bin').read_bytes()
marker=(identity+version).encode()+b'\0'
historical_marker=bytes([0x53,0x4c,0x59])+('_ARCADE_N16R8|'+version).encode()+b'\0'
if args.env == 'release' and historical_marker not in image:
    raise SystemExit('Missing compatibility marker for previously shipped OTA clients')
if marker not in image or image[0]!=0xe9 or len(image)>slot_size:
    raise SystemExit('Wrong board/version, invalid image, or firmware exceeds an OTA slot')
if image[12:14]!=b'\0\0':raise SystemExit('Firmware must target classic ESP32')
if image[3]>>4!=(2 if flash_mb == 4 else 4):raise SystemExit('Firmware header declares wrong flash size')
# Check the actual compiled partition table, including the otadata redundancy sectors.
table=(build/'partitions.bin').read_bytes()
parts={}
for offset in range(0,len(table),32):
    magic,typ,sub,address,size,label,flags=struct.unpack('<HBBII16sI',table[offset:offset+32])
    if magic!=0x50aa:break
    parts[label.split(b'\0')[0].decode()]=(typ,sub,address,size)
assert parts['app0']==(0,0x10,0x10000,slot_size)
assert parts['app1']==(0,0x11,0x10000+slot_size,slot_size)
assert parts['otadata']==(1,0,0xe000,0x2000)
assert parts['coredump']==(1,3,0x10000+2*slot_size,0x10000)
assert all(address+size <= flash_mb*1024*1024 for _,_,address,size in parts.values())
dest=root/'dist';dest.mkdir(exist_ok=True)
asset='arcade-esp32-'+suffix+'.bin'
shutil.copyfile(build/'firmware.bin',dest/asset)
manifest=dict(schema=1,board=board,version=version,asset=asset,size=len(image),sha256=hashlib.sha256(image).hexdigest())
(dest/manifest_name).write_text(json.dumps(manifest,indent=2)+'\n')
# First flash starts from a blank OTA-data area and boots app0. No filesystem is needed.
factory=bytearray(b'\xff'*(0x10000+len(image)))
for address,data in [(0x1000,(build/'bootloader.bin').read_bytes()),(0x8000,table),(0x10000,image)]:
    factory[address:address+len(data)]=data
(dest/('arcade-factory-esp32-'+suffix+'.bin')).write_bytes(factory)
(dest/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(dest.glob('*.bin'))))
print(json.dumps(manifest,indent=2))
print('Factory image: flash arcade-factory-esp32-'+suffix+'.bin at address 0x0; OTA uses the application asset only.')
