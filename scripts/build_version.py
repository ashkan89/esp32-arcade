Import("env")
from pathlib import Path
import re
version=Path(env.subst("$PROJECT_DIR"),"VERSION").read_text().strip()
if not re.fullmatch(r"\d{1,5}\.\d{1,5}\.\d{1,5}",version) or any(int(x)>65535 for x in version.split('.')):
    raise RuntimeError("VERSION must contain major.minor.patch with components <=65535")
env.Append(CPPDEFINES=[("ARCADE_VERSION",env.StringifyMacro(version))])
