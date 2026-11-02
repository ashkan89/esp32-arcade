"""PlatformIO pre-build: embed only the user's local cartridge."""
Import('env')
import subprocess
import sys
subprocess.run([sys.executable, 'scripts/import_atari_sources.py'], cwd=env['PROJECT_DIR'], check=True)
