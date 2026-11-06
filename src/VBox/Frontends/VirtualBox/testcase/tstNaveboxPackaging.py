# $Id$
"""Navebox asset, Windows packaging and translation integration checks."""

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

import ast
from pathlib import Path
import re
import struct
import unittest
import xml.etree.ElementTree as ET


GUI_DIR = Path(__file__).resolve().parents[1]
ROOT_DIR = GUI_DIR.parents[3]
WIX_NS = {'w': 'http://wixtoolset.org/schemas/v4/wxs'}


def about_sources():
    """Read literal tr() messages, including adjacent C++ string literals."""
    text = (GUI_DIR / 'src/VBoxAboutDlg.cpp').read_text(encoding='utf-8')
    literal = r'"(?:\\.|[^"\\])*"'
    return {
        ''.join(ast.literal_eval(part) for part in re.findall(literal, match.group(1)))
        for match in re.finditer(r'\btr\(\s*((?:' + literal + r'\s*)+)', text)
    }


class NaveboxPackagingTests(unittest.TestCase):
    """Check contracts between independently maintained product subsystems."""

    def test_installed_entry_points(self):
        """Every branded shortcut launches a file included in the MSI."""
        installer = ROOT_DIR / 'src/VBox/Installer/win'
        files = ET.parse(installer / 'VBoxMergeApp.wxi')
        names = {node.attrib['Name'] for node in files.findall('.//w:File', WIX_NS)}
        branding = (GUI_DIR / 'src/globals/UIBranding.h').read_text(encoding='utf-8')
        product = re.search(r'#define VBOX_GUI_PRODUCT_NAME "([^"]+)"', branding).group(1)
        app_id = re.search(r'#define VBOX_GUI_APP_USER_MODEL_ID L"([^"]+)"', branding).group(1)
        executable = product + '.exe'
        self.assertIn(executable, names)
        self.assertIn(product + '.VisualElementsManifest.xml', names)
        self.assertIn('VirtualBox.exe', names)

        package = ET.parse(installer / 'VirtualBox.wxs')
        shortcuts = [node for node in package.findall('.//w:Shortcut', WIX_NS)
                     if node.attrib['Id'] in ('sc_StartMenuVBox', 'sc_DesktopVBox')]
        self.assertEqual(len(shortcuts), 2)
        for shortcut in shortcuts:
            with self.subTest(shortcut=shortcut.attrib['Id']):
                self.assertEqual(shortcut.attrib['Name'], product)
                self.assertEqual(shortcut.attrib['Target'], '[INSTALLDIR]' + executable)
                properties = {node.attrib['Key']: node.attrib.get('Value')
                              for node in shortcut.findall('w:ShortcutProperty', WIX_NS)}
                self.assertEqual(properties.get('System.AppUserModel.ID'), app_id)
        launch = package.find('.//w:CustomAction[@Id="ca_StartVBox"]', WIX_NS)
        self.assertIsNotNone(launch)
        self.assertEqual(launch.attrib['ExeCommand'], '[INSTALLDIR]' + executable)

    def test_brand_resources(self):
        """The compiled brand resource references existing, contained assets."""
        brand_dir = GUI_DIR / 'images/Navebox'
        resource = ET.parse(GUI_DIR / 'VirtualBoxBrand.qrc')
        aliases = set()
        for node in resource.findall('.//file'):
            if '@VBOX_NAVEBOX_BRAND_DIR@' not in node.text:
                continue
            with self.subTest(alias=node.attrib['alias']):
                self.assertNotIn(node.attrib['alias'], aliases)
                aliases.add(node.attrib['alias'])
                path = Path(node.text.replace('@VBOX_NAVEBOX_BRAND_DIR@', str(brand_dir))).resolve()
                self.assertTrue(path.is_relative_to(brand_dir.resolve()))
                self.assertTrue(path.is_file(), path)
        self.assertIn('navebox.ico', aliases)
        for direction in ('up', 'down'):
            for theme in ('light', 'dark'):
                self.assertIn('navebox-chevron-' + direction + '-' + theme + '.png', aliases)

    def test_exported_icon_sizes(self):
        """PNG/ICO container sizes agree with their advertised resolutions."""
        brand_dir = GUI_DIR / 'images/Navebox'
        for path in sorted(brand_dir.glob('navebox-*.png')):
            with self.subTest(asset=path.name):
                size = int(path.stem.split('-')[-1])
                data = path.read_bytes()
                self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
                self.assertEqual(struct.unpack_from('>II', data, 16), (size, size))
        data = (brand_dir / 'navebox.ico').read_bytes()
        reserved, kind, count = struct.unpack_from('<HHH', data)
        self.assertEqual((reserved, kind), (0, 1))
        self.assertGreater(count, 0)
        end = 6 + count * 16
        resolutions = set()
        for index in range(count):
            width, height, _, _, _, _, length, offset = struct.unpack_from('<BBBBHHII', data, 6 + index * 16)
            self.assertGreaterEqual(offset, end)
            end = offset + length
            self.assertLessEqual(end, len(data))
            self.assertEqual(data[offset:offset + 8], b'\x89PNG\r\n\x1a\n')
            self.assertEqual(struct.unpack_from('>II', data, offset + 16), (width or 256, height or 256))
            resolutions.add(width or 256)
        self.assertTrue({16, 32, 48, 256}.issubset(resolutions))

    def test_bios_boot_logo(self):
        """Both product configurations embed a bitmap accepted by the BIOS loader."""
        logo = ROOT_DIR / 'src/VBox/Artwork/Navebox/navebox-bios.bmp'
        data = logo.read_bytes()
        self.assertEqual(data[:2], b'BM')
        size, _, _, offset = struct.unpack_from('<IHHI', data, 2)
        header, width, height, planes, bits, compression = struct.unpack_from('<IiiHHI', data, 14)
        self.assertEqual(size, len(data))
        self.assertEqual((header, width, height, planes, bits, compression), (40, 560, 160, 1, 24, 0))
        self.assertEqual(offset, 54)
        self.assertEqual(len(data) - offset, ((width * 3 + 3) & ~3) * height)
        self.assertEqual(data[offset:offset + 3], b'\0\0\0')
        self.assertTrue(any(data[offset:]))
        config = (ROOT_DIR / 'Config.kmk').read_text(encoding='utf-8')
        choices = re.findall(r'^\s*VBOX_BRAND_BIOS_LOGO\s*:=\s*(\S+)', config, re.MULTILINE)
        self.assertEqual(choices, ['$(PATH_ROOT)/src/VBox/Artwork/Navebox/navebox-bios.bmp'] * 2)

    def test_about_catalogs(self):
        """All catalogues expose current About messages; finished placeholders survive."""
        sources = about_sources()
        self.assertIn('Copy Build Information', sources)
        self.assertIn('Contact', sources)
        for path in sorted((GUI_DIR / 'nls').glob('VirtualBox_*.ts')):
            if path.name == 'VirtualBox_en.ts':  # English intentionally contains plural forms only.
                continue
            with self.subTest(catalog=path.name):
                tree = ET.parse(path)
                context = next((node for node in tree.findall('context')
                                if node.findtext('name') == 'VBoxAboutDlg'), None)
                self.assertIsNotNone(context)
                messages = {node.findtext('source'): node for node in context.findall('message')}
                self.assertFalse(sources - messages.keys(), sorted(sources - messages.keys()))
                for source in sources:
                    translation = messages[source].find('translation')
                    self.assertIsNotNone(translation)
                    self.assertNotIn(translation.attrib.get('type'), ('vanished', 'obsolete'))
                    if translation.attrib.get('type') != 'unfinished':
                        self.assertEqual(set(re.findall(r'%(?:[1-9][0-9]?|n)', source)),
                                         set(re.findall(r'%(?:[1-9][0-9]?|n)', ''.join(translation.itertext()))))

    def test_windows_installer_identity(self):
        """Setup product, publisher, artwork and catalog messages use the same brand."""
        config = (ROOT_DIR / 'Config.kmk').read_text(encoding='utf-8')
        self.assertRegex(config, r'(?m)^VBOX_PRODUCT\s*=\s*Navebox$')
        self.assertRegex(config, r'(?m)^VBOX_BRAND_WIN_INST_PUBLISHER\s*:=\s*Navebox$')
        installer = ROOT_DIR / 'src/VBox/Installer/win'
        package = ET.parse(installer / 'VirtualBox.wxs').getroot().find('w:Package', WIX_NS)
        self.assertEqual(package.attrib['Name'], '$(env.VBOX_PRODUCT) $(env.VBOX_VERSION_STRING)')
        self.assertEqual(package.attrib['Manufacturer'], '$(env.VBOX_BRAND_WIN_INST_PUBLISHER)')
        self.assertEqual(package.find('.//w:Directory[@Id="INSTALLDIR"]', WIX_NS).attrib['Name'], '$(env.VBOX_PRODUCT)')
        interface = ET.parse(installer / 'UserInterface.wxi')
        for identifier, variable in (('dlgjpg', 'DLGJPG'), ('bannerjpg', 'BANNER')):
            node = interface.find('.//w:Binary[@Id="' + identifier + '"]', WIX_NS)
            self.assertEqual(node.attrib['SourceFile'], '$(env.VBOX_BRAND_WIN_INST_' + variable + ')')
        for name in ('installer-dialog.jpg', 'installer-banner.jpg', 'installer-splash.png'):
            path = ROOT_DIR / 'src/VBox/Artwork/Navebox' / name
            self.assertGreater(path.stat().st_size, 100)
            self.assertIn(path.as_posix().split('/src/')[1], config)
        namespace = {'w': 'http://wixtoolset.org/schemas/v4/wxl'}
        for path in sorted((installer / 'NLS').glob('*.wxl')):
            for node in ET.parse(path).findall('w:String', namespace):
                with self.subTest(catalog=path.name, message=node.attrib['Id']):
                    self.assertNotRegex(node.attrib.get('Value', ''), r'Oracle|VirtualBox')


if __name__ == '__main__':
    unittest.main(verbosity=2)
