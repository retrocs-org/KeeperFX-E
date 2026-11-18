#!/usr/bin/env python3
"""Exercise the real engine input code using SDL virtual gamepads."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
deps = Path(os.environ.get('KFX_MAC_DEPS_ROOT', root / 'build-macos-deps'))
build = Path(os.environ.get('KFX_MAC_BUILD_DIR', root / 'build-mac-universal'))
flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--static', '--libs', 'sdl3'], text=True,
    env=dict(os.environ, PKG_CONFIG_PATH=str(deps / 'sdl-prefix/lib/pkgconfig'))))
with tempfile.TemporaryDirectory(prefix='keeperfx-controller-tests-') as folder:
    executable = Path(folder) / 'test-controllers'
    subprocess.run([
        'clang++', '-std=c++17', '-Wl,-dead_strip',
        str(root / 'macos/tests/test-controllers.cpp'),
        '-I' + str(root / 'src'), '-I' + str(deps / 'sdl-prefix/include'),
        *['-I' + str(deps / name / 'include') for name in ('sdlmix-prefix','sdlimg-prefix','openal-prefix','lua-fat','spng-prefix')],
        str(build / 'CMakeFiles/keeperfx.dir/src/bflib_input_joyst.cpp.o'),
        str(build / 'CMakeFiles/keeperfx.dir/src/kjm_input.c.o'),
        str(build / 'CMakeFiles/keeperfx.dir/src/macos_controls.c.o'),
        *flags, '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
