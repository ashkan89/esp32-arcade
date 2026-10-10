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
args=parser.parse_args()
version=(root/'VERSION').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+',version):raise SystemExit('Invalid VERSION')
if args.tag and args.tag!='v'+version:raise SystemExit('Release tag must match VERSION exactly')
build=root/'.pio/build/release'
image=(build/'firmware.bin').read_bytes()
marker=('SLY_ARCADE_N16R8|'+version).encode()+b'\0'
if marker not in image or image[0]!=0xe9 or len(image)>0x300000:
    raise SystemExit('Wrong board/version, invalid image, or firmware exceeds an OTA slot')
if image[12:14]!=b'\0\0':raise SystemExit('Firmware must target classic ESP32')
if image[3]>>4!=4:raise SystemExit('Firmware header must declare 16 MB flash')
# Check the actual compiled partition table, including the otadata redundancy sectors.
table=(build/'partitions.bin').read_bytes()
parts={}
for offset in range(0,len(table),32):
    magic,typ,sub,address,size,label,flags=struct.unpack('<HBBII16sI',table[offset:offset+32])
    if magic!=0x50aa:break
    parts[label.split(b'\0')[0].decode()]=(typ,sub,address,size)
assert parts['app0']==(0,0x10,0x10000,0x300000)
assert parts['app1']==(0,0x11,0x310000,0x300000)
assert parts['otadata']==(1,0,0xe000,0x2000)
dest=root/'dist';dest.mkdir(exist_ok=True)
asset='arcade-esp32-n16r8.bin'
shutil.copyfile(build/'firmware.bin',dest/asset)
manifest=dict(schema=1,board='esp32-wrover-n16r8-ili9225',version=version,asset=asset,size=len(image),sha256=hashlib.sha256(image).hexdigest())
(dest/'arcade-release.json').write_text(json.dumps(manifest,indent=2)+'\n')
# First flash starts from a blank OTA-data area and boots app0. No filesystem is needed.
factory=bytearray(b'\xff'*(0x10000+len(image)))
for address,data in [(0x1000,(build/'bootloader.bin').read_bytes()),(0x8000,table),(0x10000,image)]:
    factory[address:address+len(data)]=data
(dest/'arcade-factory-esp32-n16r8.bin').write_bytes(factory)
(dest/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(dest.glob('*.bin'))))
print(json.dumps(manifest,indent=2))
print('Factory image: flash arcade-factory-esp32-n16r8.bin at address 0x0; OTA uses the application asset only.')
