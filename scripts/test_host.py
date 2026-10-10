"""Build and execute pure C++ rule tests with GCC/Clang or MSVC. No Arduino mocks."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / "test-results"
out.mkdir(exist_ok=True)
sources = [root / "tests/rules.cpp", *sorted((root / "src/games").glob("*.cpp")),
           root / "src/arcade/Core.cpp", root / "src/arcade/Canvas.cpp", root / "src/arcade/Console.cpp"]
parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
compiler = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++") or shutil.which("cl")
exe = out / ("rules.exe" if os.name == "nt" else "rules")
if not compiler and os.name == "nt":
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home() / ".platformio")))
    candidate = core / "packages/toolchain-gccmingw32/bin/g++.exe"
    if candidate.exists():
        compiler = str(candidate)
        os.environ["PATH"] = str(candidate.parent) + os.pathsep + os.environ["PATH"]
if not compiler and os.name == "nt":
    install = ""
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    if vswhere.exists():
        install = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    if not install:
        candidates = sorted((Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "Microsoft Visual Studio").glob("*/*/Common7/Tools/VsDevCmd.bat"))
        if candidates:
            install = str(candidates[-1].parents[2])
    if install:
        bat = Path(install) / "Common7/Tools/VsDevCmd.bat"
        env = subprocess.check_output(f'cmd /d /s /c ""{bat}" -arch=x64 >nul && set"', text=True)
        for line in env.splitlines():
            if "=" in line and not line.startswith("="):
                key, value = line.split("=", 1)
                os.environ[key] = value
        compiler = shutil.which("cl")
        if not compiler:
            # Some standalone VS installations lack vswhere, so VsDevCmd returns early.
            toolsets = sorted((Path(install) / "VC/Tools/MSVC").glob("*"))
            sdk = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Windows Kits/10"
            versions = sorted((sdk / "Include").glob("*"))
            if toolsets and versions:
                tools, version = toolsets[-1], versions[-1].name
                compiler = str(tools / "bin/Hostx64/x64/cl.exe")
                os.environ["PATH"] = str(tools / "bin/Hostx64/x64") + os.pathsep + os.environ["PATH"]
                os.environ["INCLUDE"] = os.pathsep.join(map(str, [tools / "include", *[sdk / "Include" / version / p for p in ["ucrt", "shared", "um", "winrt"]]]))
                os.environ["LIB"] = os.pathsep.join(map(str, [tools / "lib/x64", sdk / "Lib" / version / "ucrt/x64", sdk / "Lib" / version / "um/x64"]))
if not compiler:
    sys.exit("Install a C++14 or newer compiler or run in a Visual Studio Developer terminal.")
if Path(compiler).stem.lower() == "cl":
    command = [compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/O2", "/I" + str(root / "include"), *map(str, sources), "/Fe:" + str(exe), "/Fo:" + str(out) + os.sep]
    if args.sanitize:
        command += ["/fsanitize=address", "/Zi"]
else:
    command = [compiler, "-std=c++14", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-I", str(root / "include"), *map(str, sources), "-o", str(exe)]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
build = subprocess.run(command, cwd=root, capture_output=True, text=True)
(out / "host-build.log").write_text(build.stdout + build.stderr, encoding="utf-8")
if build.returncode:
    print(build.stdout + build.stderr)
    sys.exit(build.returncode)
run = subprocess.run([str(exe)], cwd=root, capture_output=True, text=True)
(out / "host-tests.log").write_text(run.stdout + run.stderr, encoding="utf-8")
print(run.stdout + run.stderr)
sys.exit(run.returncode)
