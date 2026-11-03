# $Id$
"""Read-only MSI verification and delivery of Windows x64 Navebox artifacts."""
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
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


def package_names(output, revision):
    """Read the generated version rather than freezing a release number in scripts."""
    text = (output / 'version-generated.h').read_text(encoding='utf-8')
    match = re.search(r'^#define VBOX_VERSION_STRING "([A-Za-z0-9_.-]+)"$', text, re.MULTILINE)
    if not match:
        raise ValueError('Missing generated version; run build.cmd compile first.')
    base = f'Navebox-{match.group(1)}-r{revision}'
    return (base + '.msi', base + '-MultiArch.exe')


def executable_identity(filename):
    """Read the bootstrapper's Windows file properties without executing it."""
    filename = filename.resolve()
    api = ctypes.WinDLL('version')
    api.GetFileVersionInfoSizeW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD)]
    api.GetFileVersionInfoSizeW.restype = wintypes.DWORD
    api.GetFileVersionInfoW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    api.GetFileVersionInfoW.restype = wintypes.BOOL
    api.VerQueryValueW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_void_p),
                                 ctypes.POINTER(wintypes.UINT)]
    api.VerQueryValueW.restype = wintypes.BOOL
    ignored = wintypes.DWORD()
    size = api.GetFileVersionInfoSizeW(str(filename), ctypes.byref(ignored))
    if not size:
        raise ValueError('Installer EXE has no version resource')
    block = ctypes.create_string_buffer(size)
    if not api.GetFileVersionInfoW(str(filename), 0, size, block):
        raise ValueError('Cannot read installer EXE version resource')
    result = {}
    for key in ('ProductName', 'FileDescription', 'CompanyName'):
        value = ctypes.c_void_p()
        length = wintypes.UINT()
        if not api.VerQueryValueW(block, '\\StringFileInfo\\040904b0\\' + key,
                                 ctypes.byref(value), ctypes.byref(length)):
            raise ValueError('Installer EXE is missing ' + key)
        result[key] = ctypes.wstring_at(value.value, length.value - 1)
    return result


class MsiDatabase:
    """Own read-only Windows Installer database and query handles."""

    def __init__(self, filename):
        self.api = ctypes.WinDLL('msi')
        self.handle = wintypes.UINT()
        signatures = {
            'MsiOpenDatabaseW': [wintypes.LPCWSTR, wintypes.LPCWSTR, ctypes.POINTER(wintypes.UINT)],
            'MsiDatabaseOpenViewW': [wintypes.UINT, wintypes.LPCWSTR, ctypes.POINTER(wintypes.UINT)],
            'MsiViewExecute': [wintypes.UINT, wintypes.UINT],
            'MsiViewFetch': [wintypes.UINT, ctypes.POINTER(wintypes.UINT)],
            'MsiRecordGetFieldCount': [wintypes.UINT],
            'MsiRecordGetStringW': [wintypes.UINT, wintypes.UINT, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)],
            'MsiRecordReadStream': [wintypes.UINT, wintypes.UINT, ctypes.c_void_p, ctypes.POINTER(wintypes.DWORD)],
            'MsiCloseHandle': [wintypes.UINT],
        }
        for name, arguments in signatures.items():
            getattr(self.api, name).argtypes = arguments
            getattr(self.api, name).restype = wintypes.UINT
        self.checked(self.api.MsiOpenDatabaseW(str(filename), None, ctypes.byref(self.handle)))

    @staticmethod
    def checked(code):
        """Propagate a failed MSI operation, including the native error code."""
        if code:
            raise OSError(code, 'Windows Installer read-only query failed')

    def close(self):
        """Release the database without committing or executing installer actions."""
        self.api.MsiCloseHandle(self.handle)

    def query(self, sql, stream=False):
        """Fetch text rows or a single binary stream using fixed internal SQL."""
        view = wintypes.UINT()
        self.checked(self.api.MsiDatabaseOpenViewW(self.handle, sql, ctypes.byref(view)))
        try:
            self.checked(self.api.MsiViewExecute(view, 0))
            rows = []
            while True:
                record = wintypes.UINT()
                code = self.api.MsiViewFetch(view, ctypes.byref(record))
                if code == 259:
                    return rows
                self.checked(code)
                try:
                    if stream:
                        chunks = []
                        while True:
                            size = wintypes.DWORD(1024 * 1024)
                            buffer = ctypes.create_string_buffer(size.value)
                            self.checked(self.api.MsiRecordReadStream(record, 1, buffer, ctypes.byref(size)))
                            if not size.value:
                                break
                            chunks.append(buffer.raw[:size.value])
                        rows.append(b''.join(chunks))
                    else:
                        row = []
                        for index in range(1, self.api.MsiRecordGetFieldCount(record) + 1):
                            size = wintypes.DWORD()
                            code = self.api.MsiRecordGetStringW(record, index, None, ctypes.byref(size))
                            if code not in (0, 234):
                                self.checked(code)
                            size.value += 1
                            buffer = ctypes.create_unicode_buffer(size.value)
                            self.checked(self.api.MsiRecordGetStringW(record, index, buffer, ctypes.byref(size)))
                            row.append(buffer.value)
                        rows.append(row)
                finally:
                    self.api.MsiCloseHandle(record)
        finally:
            self.api.MsiCloseHandle(view)


def verify_msi(root, output, package, work):
    """Check branding and extract payload for comparison without starting setup."""
    database = MsiDatabase(package)
    try:
        properties = dict(database.query('SELECT `Property`, `Value` FROM `Property`'))
        if not properties.get('ProductName', '').startswith('Navebox '):
            raise ValueError('MSI ProductName is not branded Navebox')
        if properties.get('Manufacturer') != 'Navebox':
            raise ValueError('MSI distributor is not Navebox')
        controls = database.query('SELECT `Dialog_`, `Control`, `Type`, `Text` FROM `Control`')
        for dialog, name, kind, text in controls:
            # The original component license and copyright notices must remain intact.
            if kind != 'ScrollableText' and re.search(r'VirtualBox|Oracle', text):
                raise ValueError(f'Unbranded installer control: {dialog}/{name}')
        for identifier, source in (('dlgjpg', 'installer-dialog.jpg'), ('bannerjpg', 'installer-banner.jpg')):
            streams = database.query(f"SELECT `Data` FROM `Binary` WHERE `Name`='{identifier}'", stream=True)
            if streams != [(root / 'src/VBox/Artwork/Navebox' / source).read_bytes()]:
                raise ValueError('MSI contains stale artwork: ' + identifier)
        files = database.query('SELECT `File`, `FileName`, `FileSize` FROM `File`')
        names = {name.split('|')[-1].lower(): identifier for identifier, name, _ in files}
        required = ('Navebox.exe', 'VirtualBox.exe', 'VirtualBoxVM.exe', 'VBoxC.dll', 'VBoxSVC.exe',
                    'VBoxSDS.exe', 'UICommon.dll', 'VBoxRT.dll', 'VBoxVMM.dll', 'VBoxDD.dll',
                    'VBoxSup.sys', 'VBoxSup.inf', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'qwindows.dll')
        for name in required:
            if name.lower() not in names:
                raise ValueError('MSI is missing ' + name)
        cabinet = database.query("SELECT `Data` FROM `_Streams` WHERE `Name`='product.cab'", stream=True)
        if len(cabinet) != 1:
            raise ValueError('Expected one embedded product.cab')
        export = work / 'payload-check'
        export.mkdir(exist_ok=True)
        source = export / 'product.cab'
        source.write_bytes(cabinet[0])
    finally:
        database.close()
    with (work / 'cabinet-extraction.log').open('w', encoding='utf-8') as log:
        subprocess.run(['expand.exe', '-F:*', str(source), str(export)], stdout=log,
                       stderr=subprocess.STDOUT, check=True)
    for identifier, _, size in files:
        path = (export / identifier).resolve()
        if not path.is_relative_to(export.resolve()) or path.stat().st_size != int(size):
            raise ValueError('Invalid extracted file: ' + identifier)
    for name in ('Navebox.exe', 'UICommon.dll', 'VBoxRT.dll', 'VBoxVMM.dll', 'VBoxDD.dll'):
        if (export / names[name.lower()]).read_bytes() != (output / 'bin' / name).read_bytes():
            raise ValueError('MSI payload differs from current output: ' + name)
    logo = (root / 'src/VBox/Artwork/Navebox/navebox-bios.bmp').read_bytes()
    if logo not in (export / names['vboxdd.dll']).read_bytes():
        raise ValueError('MSI is missing the default Navebox BIOS bitmap')
    license_data = (export / names['license_en_us.rtf']).read_bytes()
    if license_data != (root / 'doc/License-gpl-3.0.rtf').read_bytes():
        raise ValueError('Original component licensing notices have changed')
    return {'product_name': properties['ProductName'], 'publisher': properties['Manufacturer'],
            'extracted_files': len(files), 'msi_artwork_matches': True,
            'core_payload_matches': True, 'upstream_license_preserved': True,
            'bios_logo_sha256': hashlib.sha256(logo).hexdigest()}


def verify_and_deliver(root, output, names, work, provenance=None):
    """Publish verified local build copies and hashes under the ignored output tree."""
    result = verify_msi(root, output, output / 'bin' / names[0], work)
    executable = output / 'bin' / names[1]
    identity = executable_identity(executable)
    if identity != {'ProductName': 'Navebox', 'FileDescription': 'Navebox Installer', 'CompanyName': 'Navebox'}:
        raise ValueError('Installer EXE has stale product metadata: ' + repr(identity))
    splash = (root / 'src/VBox/Artwork/Navebox/installer-splash.png').read_bytes()
    if splash not in executable.read_bytes():
        raise ValueError('Installer EXE contains a stale splash image')
    delivery = output / 'delivery'
    delivery.mkdir(exist_ok=True)
    version = result['product_name'].removeprefix('Navebox ')
    files = []
    for source, suffix in zip(names, ('.msi', '-setup.exe')):
        target = delivery / f'Navebox-{version}-windows-x64-test{suffix}'
        shutil.copy2(output / 'bin' / source, target)
        files.append({'file': target.name, 'bytes': target.stat().st_size,
                      'sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    result.update({'files': files, 'release_qualified': False, 'build_type': 'release',
                   'exe_identity': identity, 'exe_splash_matches': True, 'provenance': provenance or {},
                   'firmware_branding': 'BIOS updated; prebuilt EFI unchanged',
                   'qualification': 'Signing/trust, clean-host install/upgrade and HM/NEM workloads require separate qualification.',
                   'shared_upgrade_code': '{C4BAD770-BFE8-4D2C-A592-693028A7215B}'})
    (delivery / 'build-manifest.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    (delivery / 'SHA256SUMS.txt').write_text(''.join(f"{item['sha256']}  {item['file']}\n" for item in files), encoding='utf-8')
    (delivery / 'README-FIRST.txt').write_text('''NAVEBOX WINDOWS X64 - LOCAL TEST BUILD

These files are release-optimized local build artifacts, not a qualified public
release. Signing is controlled by LocalConfig; this helper does not establish
Microsoft kernel-driver trust. Local test certificates may prevent driver
installation/loading on normal Windows. No security settings are changed by
this build helper. Test installation on a disposable system.

COM, services and MSI upgrade identity are shared with VirtualBox. Installing
this package can upgrade/replace an existing VirtualBox installation.

Use the complete installer, not Navebox.exe alone. The destination needs the
matching Microsoft Visual C++ v14 runtime and a usable virtualization backend.
It does not need the compiler or Qt SDK. Included optional features depend on
LocalConfig; the host-only test profile omits Guest Additions and the manual.

Original upstream copyright and licensing notices are retained. The default
BIOS splash is Navebox; prebuilt EFI firmware is unchanged.
See build-manifest.json and SHA256SUMS.txt for validation and integrity data.
''', encoding='utf-8')
    print(f'Verified {result["extracted_files"]} payload files. Delivery: {delivery}', flush=True)
