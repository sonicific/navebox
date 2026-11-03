# $Id$
"""Regression checks for build configuration boundaries and package naming."""
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

import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from build_environment import build_arguments, load_settings
from package_windows import package_names


class BuildConfigurationTests(unittest.TestCase):
    """Reject ambiguous build inputs before invoking external toolchains."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.data = {key: 'toolchain/' + key for key in
                     ('local_config', 'msvc_root', 'windows_sdk_root', 'qt_root',
                      'wix_root', 'nasm_root', 'yasm_root', 'xml_tools_root')}
        self.data.update(windows_sdk_version='10.0.22621.0', svn_revision=0)

    def write_config(self):
        """Create a disposable machine configuration independent of host SDKs."""
        (self.root / 'config.json').write_text(json.dumps(self.data), encoding='utf-8')
        return load_settings(self.root, 'config.json')

    def test_relative_paths_are_repo_relative(self):
        settings = self.write_config()
        self.assertEqual(settings['local_config'], self.root / 'toolchain/local_config')
        with patch('build_environment.make_path', side_effect=lambda path: str(path)):
            arguments = build_arguments(self.root, settings, 3)
        self.assertIn('KBUILD_TYPE=release', arguments)
        self.assertIn('LOCALCFG=' + settings['local_config'].as_posix(), arguments)
        self.assertIn('VBOX_SVN_REV=0', arguments)
        self.assertFalse(any('DRIVERLESS' in value or 'WITHOUT_HARDENING' in value for value in arguments))

    def test_revision_is_explicit_integer(self):
        for revision in (True, -1, '123'):
            with self.subTest(revision=revision):
                self.data['svn_revision'] = revision
                with self.assertRaises(ValueError):
                    self.write_config()

    def test_invalid_sdk_version_is_rejected(self):
        self.data['windows_sdk_version'] = '../other'
        with self.assertRaises(ValueError):
            self.write_config()

    def test_package_names_follow_generated_version(self):
        header = self.root / 'version-generated.h'
        header.write_text('#define VBOX_VERSION_STRING "8.1.5_BETA1"\n', encoding='utf-8')
        self.assertEqual(package_names(self.root, 42),
                         ('Navebox-8.1.5_BETA1-r42.msi', 'Navebox-8.1.5_BETA1-r42-MultiArch.exe'))
        header.write_text('#define VBOX_VERSION_STRING "../../escape"\n', encoding='utf-8')
        with self.assertRaises(ValueError):
            package_names(self.root, 42)


if __name__ == '__main__':
    unittest.main(verbosity=2)
