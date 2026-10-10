"""Run the complete supported firmware build matrix and host tests, retaining logs."""
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / "test-results"
out.mkdir(exist_ok=True)
pio = shutil.which("pio") or shutil.which("platformio")
if not pio:
    sys.exit("Install PlatformIO Core: python -m pip install platformio==6.2.0")
environments = ["migration", "release", "debug", "no_psram", "one_button", "expanded", "xbox"]
command = [pio, "run"]
for name in environments:
    command += ["-e", name]
with (out / "firmware-builds.log").open("w", encoding="utf-8") as log:
    process = subprocess.Popen(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    for line in process.stdout:
        log.write(line)
        if any(term in line for term in ["Processing ", "error:", "SUCCESS", "FAILED", "RAM:", "Flash:", "Environment", "migration ", "release ", "debug ", "no_psram ", "one_button ", "expanded ", "xbox "]):
            print(line, end="", flush=True)
    if process.wait():
        sys.exit(process.returncode)
subprocess.run([sys.executable, "scripts/test_host.py"], cwd=root, check=True)
subprocess.run([sys.executable, "scripts/preview.py"], cwd=root, check=True)
