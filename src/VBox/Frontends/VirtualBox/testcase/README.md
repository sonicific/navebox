<!-- $Id$ -->
<!-- Copyright (C) 2026 Oracle and/or its affiliates. SPDX-License-Identifier: GPL-3.0-only -->

# Navebox frontend regression checks

Run `tstNaveboxPackaging.py` with Python 3.9 or newer from any working
directory. It uses only the Python standard library and does not require SVN,
Git metadata, fetched dependencies or a running VirtualBox installation.

Build `tstNaveboxGui` through the frontend `Makefile.kmk` with
`VBOX_WITH_TESTCASES=1`. It links against the matching UICommon library and
installed resources. In the prepared developer environment:

```text
kmk -C src/VBox/Frontends/VirtualBox VBOX_WITH_TESTCASES=1 tstNaveboxGui
out/<host>.<arch>/<build-type>/bin/testcase/tstNaveboxGui
```

Windows uses the `.exe` suffix. Put the matching output `bin` directory in
`PATH`; on other hosts make the corresponding shared-library directory
available to the loader. Qt's offscreen platform plugin must be deployed or
available via `QT_QPA_PLATFORM_PLUGIN_PATH`. The test selects offscreen unless
`QT_QPA_PLATFORM` is explicitly set. It does not connect to VBoxSVC or touch VM
settings. Clipboard assertions run only on offscreen platforms.

Set `VBOX_GUI_TEST_SNAPSHOT_DIR` to a writable directory to export the real
About pages in light and dark palettes for visual review. No images are
written when this variable is absent. An offscreen platform without system
font discovery can load a font file specified by `VBOX_GUI_TEST_FONT`. Make
the image-format plugins available through the normal deployment layout or
`QT_PLUGIN_PATH`; the test verifies the application icon can actually decode.

The checks cover runtime-state icon semantics, external icon fallback, live
translation and palette updates, build-info copying, dialog/accessibility
lifetime, control geometry, owned/ownerless modal centering and startup-error
presentation/cleanup. Centering checks compare the actual decorated window
frame with its owner's visible frame area or the available screen, allowing
screen-edge clamping. Run them with native Windows as well as scaled offscreen
platforms. They are regression tests, not substitutes for native Windows
UIAutomation, screen-reader, installer or hypervisor testing.
See `doc/navebox-release.md` at the repository root for the release matrix.

Build and run `tstNaveboxPhone` alongside `tstNaveboxGui` to exercise the
Android phone SDK adapter and manager. It copies its own executable into a
temporary mock SDK, runs actual asynchronous subprocesses, and verifies SDK
validation, literal Unicode/metacharacter arguments, phone creation and boot
state, scoped ADB commands, identity changes, failure handling and catalog
containment. It never connects to an actual ADB server or touches user AVDs.
On Windows, stage the test executable and the matching frontend/Qt runtime
DLLs together in a separate test runtime directory if the host loader does
not search `PATH` for private DLLs. The mock tools copy these local DLLs into
their temporary SDK tool directories.
Set `VBOX_GUI_TEST_SNAPSHOT_DIR` to export SDK setup and both phone-manager tabs.
Set `VBOX_GUI_TEST_STYLE=Windows` and `VBOX_GUI_TEST_NO_APP_STYLE=1` to check
phone-manager geometry without the shared application stylesheet. The default
is Fusion with the shared stylesheet. These options affect only this test.
See
`doc/navebox-phones.md` for the separate real-Android qualification gate.
