# $Id$
"""Navebox build entry point: check, doctor, compile, package, or release."""
#
# Copyright (C) 2026 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# SPDX-License-Identifier: GPL-3.0-only
#

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys

from build_environment import build_arguments, environment, load_settings, required_files


ROOT = Path(__file__).resolve().parents[2]


def run_logged(command, name, work, env):
    """Save tool output and propagate failure without running later pipeline stages."""
    logfile = work / (name + '.log')
    print(f'{name}: {logfile}', flush=True)
    with logfile.open('w', encoding='utf-8') as stream:
        with subprocess.Popen(command, cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT) as process:
            while True:
                try:
                    code = process.wait(timeout=20)
                    break
                except subprocess.TimeoutExpired:
                    print(f'{name}: running...', flush=True)
    if code:
        raise RuntimeError(f'{name} failed with exit {code}. See {logfile}')


def stage_qt(data, binary, work, env):
    """Stage runtime/plugins without registering components or installing anything."""
    qt = data['qt_root']
    run_logged([str(qt / 'bin/windeployqt.exe'), '--release', '--no-compiler-runtime',
                '--no-translations', '--dir', str(binary), str(binary / 'Navebox.exe'),
                str(binary / 'UICommon.dll')], 'qt-deployment', work, env)
    plugins = ('platforms/qminimal.dll', 'platforms/qoffscreen.dll', 'platforms/qwindows.dll',
               'sqldrivers/qsqlite.dll', 'styles/qmodernwindowsstyle.dll', 'imageformats/qico.dll',
               'imageformats/qjpeg.dll', 'imageformats/qsvg.dll', 'tls/qschannelbackend.dll',
               'networkinformation/qnetworklistmanager.dll')
    for name in plugins:
        target = binary / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(qt / 'plugins' / name, target)
    (binary / 'nls').mkdir(exist_ok=True)
    for source in (qt / 'translations').glob('qt_*.qm'):
        shutil.copy2(source, binary / 'nls' / source.name)


def main(argv=None):
    """Execute the requested pipeline, reporting prerequisites before compilation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('check', 'doctor', 'compile', 'package', 'release'), nargs='?', default='release')
    parser.add_argument('--config', default='navebox-build.local.json')
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 1, 8))
    options = parser.parse_args(argv)
    if options.jobs < 1:
        parser.error('--jobs must be positive')
    if options.action == 'check':
        subprocess.run([sys.executable, str(ROOT / 'src/VBox/Frontends/VirtualBox/testcase/tstNaveboxPackaging.py')],
                       cwd=ROOT, check=True)
        subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', str(ROOT / 'tools/navebox'),
                        '-p', 'test_*.py', '-v'], cwd=ROOT, check=True)
        return 0
    if os.name != 'nt':
        raise ValueError('This helper builds Windows x64. Use the normal kBuild workflow on other hosts.')
    data = load_settings(ROOT, options.config)
    missing = []
    for label, path in required_files(ROOT, data).items():
        exists = path.is_file()
        print(f'{label}: {"OK" if exists else "MISSING"}', flush=True)
        if not exists:
            missing.append(str(path))
    if missing:
        raise ValueError('Missing prerequisites:\n' + '\n'.join(missing))
    if options.action == 'doctor':
        print('Tool paths are present. LocalConfig, signing and release qualification still require review.')
        return 0
    work = ROOT / 'prebuild/navebox-build'
    work.mkdir(parents=True, exist_ok=True)
    env = environment(ROOT, data, work)
    command = build_arguments(ROOT, data, options.jobs)
    if options.action in ('compile', 'release'):
        run_logged(command, 'release-compile', work, env)
    if options.action in ('package', 'release'):
        from package_windows import package_names, verify_and_deliver
        output = ROOT / 'out/win.amd64/release'
        binary = output / 'bin'
        stage_qt(data, binary, work, env)
        names = package_names(output, data['svn_revision'])
        run_logged(command + [(binary / name).as_posix() for name in names], 'release-package', work, env)
        try:
            source_head = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, capture_output=True, text=True, check=False)
            mirror_head = source_head.stdout.strip() if source_head.returncode == 0 else None
        except OSError:
            mirror_head = None
        provenance = {'svn_revision': data['svn_revision'], 'windows_sdk_version': data['windows_sdk_version'],
                      'local_config_sha256': hashlib.sha256(data['local_config'].read_bytes()).hexdigest(),
                      'mirror_head': mirror_head,
                      'working_tree': 'May contain local changes; this is not a clean-source release attestation.'}
        verify_and_deliver(ROOT, output, names, work, provenance)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'Build failed: {error}', file=sys.stderr)
        sys.exit(1)
