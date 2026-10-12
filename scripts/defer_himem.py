"""Suppress IDF 4.4's premature himem constructor.

The pinned Arduino SDK enables bankswitching without SPIRAM_BOOT_INIT.
Its himem constructor therefore aborts before initArduino(). Link a local
copy of that SDK object with its .ctors section renamed. All SDK code
and the installed framework archive remain unchanged. Frame history has been removed;
retain this boot-loop safeguard for any SDK bank-switch support that remains linked.
"""
Import("env")
from pathlib import Path
import subprocess
import re
import struct

framework = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
toolchain = Path(env.PioPlatform().get_package_dir("toolchain-xtensa-esp32"))
archive = framework / "tools/sdk/esp32/lib/libesp_hw_support.a"
output = Path(env.subst("$BUILD_DIR")) / "deferred-himem"
output.mkdir(parents=True, exist_ok=True)
member = "esp_himem.c.obj"
original = output / member
patched = output / "esp_himem_deferred.o"

def tool(name):
    path = toolchain / "bin" / ("xtensa-esp32-elf-" + name)
    return str(path.with_suffix(".exe") if path.with_suffix(".exe").exists() else path)

subprocess.run([tool("ar"), "x", str(archive), member], cwd=output, check=True)
sections = subprocess.check_output([tool("objdump"), "-h", str(original)], text=True)
if ".ctors" not in sections:
    raise RuntimeError("SDK himem constructor changed; review deferred initialization")
subprocess.run([tool("objcopy"), "--rename-section", ".ctors=.arcade_deferred_himem_ctor",
                str(original), str(patched)], check=True)
# This explicit object precedes SDK libraries, preventing extraction of the
# original member (including its constructor) from libesp_hw_support.a.
env.Append(LINKFLAGS=[str(patched.resolve())])
env.Depends("$BUILD_DIR/${PROGNAME}.elf", str(patched.resolve()))
env.Depends("$BUILD_DIR/${PROGNAME}.elf", env.subst("$PROJECT_DIR/scripts/defer_himem.py"))

def verify_constructor_table(target, source, env):
    elf = str(target[0])
    listing = subprocess.check_output([tool("nm"), "--defined-only", elf], text=True)
    symbols = {}
    for line in listing.splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0], 16)
    start, end = symbols["__init_array_start"], symbols["__init_array_end"]
    dump = subprocess.check_output([tool("objdump"), "-s", "--start-address=" + hex(start),
                                    "--stop-address=" + hex(end), elf], text=True)
    data = bytearray()
    for line in dump.splitlines():
        match = re.match(r"^\s*[0-9a-f]+\s+((?:[0-9a-f]{8}\s*){1,4})", line)
        if match:
            data.extend(bytes.fromhex(match.group(1)))
    if len(data) != end - start:
        raise RuntimeError("Unable to verify SDK constructor table")
    constructors = struct.unpack("<" + "I" * (len(data) // 4), data)
    if symbols.get("esp_himem_init") in constructors:
        raise RuntimeError("Premature himem constructor would cause a PSRAM boot loop")
    print("Verified: no premature himem constructor (frame history removed)")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", verify_constructor_table)
