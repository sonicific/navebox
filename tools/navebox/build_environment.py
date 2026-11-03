# $Id$
"""Machine-local inputs for the Windows x64 Navebox build."""
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

import ctypes
import json
import os
from pathlib import Path
import re
import sys


def load_settings(root, filename):
    """Resolve explicit tool paths without saving machine settings to source files."""
    filename = Path(filename)
    if not filename.is_absolute():
        filename = root / filename
    if not filename.is_file():
        raise ValueError('Copy navebox-build.example.json to navebox-build.local.json and edit its paths first.')
    data = json.loads(filename.read_text(encoding='utf-8-sig'))
    paths = ('local_config', 'msvc_root', 'windows_sdk_root', 'qt_root', 'wix_root',
             'nasm_root', 'yasm_root', 'xml_tools_root')
    for key in paths:
        value = Path(os.path.expandvars(data[key])).expanduser()
        data[key] = (value if value.is_absolute() else root / value).resolve()
    revision = data['svn_revision']
    if isinstance(revision, bool) or not isinstance(revision, int) or revision < 0:
        raise ValueError('svn_revision must be a nonnegative integer; use 0 only for an unversioned test build.')
    if not re.fullmatch(r'\d+\.\d+\.\d+\.\d+', data['windows_sdk_version']):
        raise ValueError('windows_sdk_version must be a four-part SDK version.')
    return data


def required_files(root, data):
    """Return the actual compiler, SDK and packaging entry points we use."""
    sdk = data['windows_sdk_root']
    version = data['windows_sdk_version']
    return {
        'LocalConfig': data['local_config'],
        'kBuild': root / 'kBuild/kBuild/bin/win.amd64/kmk.exe',
        'MSVC x64': data['msvc_root'] / 'bin/Hostx64/x64/cl.exe',
        'MSVC x86': data['msvc_root'] / 'bin/Hostx64/x86/cl.exe',
        'Windows SDK': sdk / f'Include/{version}/um/windows.h',
        'Resource compiler': sdk / f'bin/{version}/x64/rc.exe',
        'SignTool': sdk / f'bin/{version}/x64/signtool.exe',
        'Qt deployment': data['qt_root'] / 'bin/windeployqt.exe',
        'Qt Windows plugin': data['qt_root'] / 'plugins/platforms/qwindows.dll',
        'WiX 4': data['wix_root'] / 'tools/net472/x86/wix.exe',
        'WiX Util extension': data['wix_root'] / 'wixext4/WixToolset.Util.wixext.dll',
        'WiX UI extension': data['wix_root'] / 'wixext4/WixToolset.Ui.wixext.dll',
        'NASM': data['nasm_root'] / 'nasm.exe',
        'YASM': data['yasm_root'] / 'yasm.exe',
        'XSLT': data['xml_tools_root'] / 'xsltproc.exe',
        'XML lint': data['xml_tools_root'] / 'xmllint.exe',
    }


def make_path(path):
    """Use Windows short paths where kBuild tool definitions cannot quote spaces."""
    value = str(path)
    if os.name == 'nt' and ' ' in value:
        function = ctypes.WinDLL('kernel32').GetShortPathNameW
        function.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint]
        function.restype = ctypes.c_uint
        size = function(value, None, 0)
        if size:
            buffer = ctypes.create_unicode_buffer(size)
            if function(value, buffer, size):
                value = buffer.value
        if ' ' in value:
            raise ValueError('kBuild requires a tool path without spaces or an available Windows short path: ' + value)
    return value.replace('\\', '/')


def environment(root, data, work):
    """Prepare a child environment without modifying system environment or security."""
    excluded = {'PATH', 'INCLUDE', 'LIB', 'TEMP', 'TMP', 'KBUILD_TYPE'}
    env = {key: value for key, value in os.environ.items() if key.upper() not in excluded}
    sdk = data['windows_sdk_root']
    version = data['windows_sdk_version']
    tools = [data['msvc_root'] / 'bin/Hostx64/x64', sdk / f'bin/{version}/x64',
             data['qt_root'] / 'bin', data['xml_tools_root'], data['nasm_root'],
             data['yasm_root'], root / 'kBuild/kBuild/bin/win.amd64']
    env['PATH'] = os.pathsep.join(map(str, tools)) + os.pathsep + os.environ.get('PATH', '')
    env['INCLUDE'] = os.pathsep.join(map(str, [data['msvc_root'] / 'include',
        *[sdk / f'Include/{version}' / part for part in ('ucrt', 'shared', 'um', 'winrt', 'cppwinrt')]]))
    env['LIB'] = os.pathsep.join(map(str, [data['msvc_root'] / 'lib/x64',
        sdk / f'Lib/{version}/ucrt/x64', sdk / f'Lib/{version}/um/x64']))
    env['TEMP'] = env['TMP'] = str(work)
    env['KBUILD_PATH'] = (root / 'kBuild/kBuild').as_posix()
    return env


def build_arguments(root, data, jobs):
    """Keep release compilation and packaging on the same local configuration."""
    sdk = make_path(data['windows_sdk_root'])
    vc = make_path(data['msvc_root'])
    qt = make_path(data['qt_root'])
    return [str(root / 'kBuild/kBuild/bin/win.amd64/kmk.exe'), f'-j{jobs}',
            'KBUILD_TYPE=release', 'KBUILD_TARGET=win', 'KBUILD_TARGET_ARCH=amd64',
            'PATH_OUT_BASE=' + make_path(root / 'out'),
            'PATH_OUT=' + make_path(root / 'out/win.amd64/release'),
            'LOCALCFG=' + data['local_config'].as_posix(), f'VBOX_SVN_REV={data["svn_revision"]}',
            'VBOX_VCC_TOOL_STEM=VCC143', 'PATH_TOOL_VCC143=' + vc,
            'PATH_TOOL_VCC143AMD64=' + vc, 'PATH_TOOL_VCC143X86=' + vc,
            'PATH_SDK_WINSDK10=' + sdk, 'SDK_WINSDK10_VERSION=' + data['windows_sdk_version'],
            'VBOX_PATH_QT=' + qt, 'PATH_SDK_QT6=' + qt, 'PATH_TOOL_QT6=' + qt,
            'VBOX_PATH_WIX=' + make_path(data['wix_root']),
            'PATH_TOOL_NASM=' + make_path(data['nasm_root']),
            'PATH_TOOL_YASM=' + make_path(data['yasm_root']),
            'VBOX_BLD_PYTHON=' + make_path(Path(sys.executable)),
            'VBOX_XSLTPROC=' + make_path(data['xml_tools_root'] / 'xsltproc.exe'),
            'VBOX_XMLLINT=' + make_path(data['xml_tools_root'] / 'xmllint.exe')]
