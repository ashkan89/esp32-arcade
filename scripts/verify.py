"""Run the complete supported firmware build matrix and host tests, retaining logs."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / "test-results"
out.mkdir(exist_ok=True)
pio = shutil.which("pio") or shutil.which("platformio")
if not pio:
    sys.exit("Install PlatformIO Core: python -m pip install platformio==6.2.0")
environments = ["release", "debug"]
command = [pio, "run"]
for name in environments:
    command += ["-e", name]
with (out / "firmware-builds.log").open("w", encoding="utf-8") as log:
    process = subprocess.Popen(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    for line in process.stdout:
        log.write(line)
        if any(term in line for term in ["Processing ", "error:", "SUCCESS", "FAILED", "RAM:", "Flash:", "Environment", "release ", "debug "]):
            print(line, end="", flush=True)
    if process.wait():
        sys.exit(process.returncode)
subprocess.run([sys.executable, "scripts/test_host.py"], cwd=root, check=True)
subprocess.run([sys.executable, "scripts/preview.py"], cwd=root, check=True)
subprocess.run([sys.executable, "scripts/preview.py", "--menus"], cwd=root, check=True)
subprocess.run([sys.executable, "scripts/package_release.py"], cwd=root, check=True)
log=(out/'firmware-builds.log').read_text(encoding='utf-8')
summary={}
for name in environments:
    section=log.split('Processing '+name+' ',1)[1].split('Processing ',1)[0]
    sizes={kind:int(re.search(kind+r':.*used (\d+) bytes',section).group(1)) for kind in ['RAM','Flash']}
    image=root/'.pio/build'/name/'firmware.bin'
    summary[name]=dict(status='PASS',static_ram_bytes=sizes['RAM'],flash_bytes=sizes['Flash'],binary_bytes=image.stat().st_size,firmware_sha256=hashlib.sha256(image.read_bytes()).hexdigest())
(out/'firmware-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
