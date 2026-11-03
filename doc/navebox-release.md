<!-- $Id$ -->
<!-- Copyright (C) 2026 Oracle and/or its affiliates. SPDX-License-Identifier: GPL-3.0-only -->

# Navebox release and validation guide

Navebox changes the VirtualBox desktop frontend while retaining internal
service, API and VM-file identifiers. Build and qualify the complete set of
matching binaries. Do not replace individual GUI DLLs in another installation.
These instructions define required evidence; they do not certify an existing
artifact as production-ready.

## Build provenance and supported scope

Choose the source baseline and record both the mirror commit and the canonical
SVN revision when available. A larger development version number does not imply
a newer qualified release. Odd build numbers in `Version.kmk` are development
builds. About shows that status and provides Copy Build Information, containing
product/upstream versions, source revision, build type, host target and Qt
version. The copied text excludes VM configuration, paths and credentials.

Prepare an external developer environment using `README.md` and the official
host-specific build instructions. Internal developers use the repository's
environment scripts. Keep machine paths and developer workarounds in
`LocalConfig.kmk`; never copy one developer's file into a release configuration.
Use `KBUILD_*` variables. Build release artifacts with `KBUILD_TYPE=release` in a
separate output tree, and retain the exact toolchain/dependency versions,
configuration options, artifact checksums and build/test logs in the release
manifest. Resolve configure/tool detection failures before claiming a clean
build is reproducible.

State which host OS/architecture, execution backend and guest configurations
are supported. Forced driverless execution deliberately follows a different
hardening path from the support-driver configuration; do not change hardening
flags without reviewing the chosen backend's requirements. Test each backend
that the release claims to support. Document unsupported integrations and
provide matching Guest Additions, or a tested compatibility policy.

## Automated checks

The standard-library packaging checks run without SDKs or a VM:

```text
python src/VBox/Frontends/VirtualBox/testcase/tstNaveboxPackaging.py
```

They check that branded MSI shortcuts launch a packaged executable and use the
process AppUserModelID; brand resources exist; PNG/ICO metadata is consistent;
and all translation catalogs expose the current About strings. The
`Navebox integration checks` GitHub workflow runs these checks. It does not
compile the hypervisor or qualify a Windows installer.

In a configured build environment, build the GUI regression executable:

```text
kmk -C src/VBox/Frontends/VirtualBox VBOX_WITH_TESTCASES=1 tstNaveboxGui
```

Run `tstNaveboxGui` from the platform's `bin/testcase` output, with the matching
`bin` directory and Qt dependencies in the platform's library search path.
The test defaults to Qt's offscreen platform and never creates a COM session,
registers a VM or reads a user's VM configuration. It checks mouse-state
distinctions through the installed icon loader at multiple sizes/scales,
custom-icon fallback, live About translations, tab preservation, palette
changes, build-info copying, repeated destruction/accessibility-cache cleanup
and styled spinbox geometry. It also checks owned/ownerless dialog frame
centering for normal, maximized and oversized owners, and the neutral
startup-error window, including close/lifetime
behavior. Native-platform runs skip the clipboard check.
Offscreen success must be supplemented by native accessibility and DPI tests.

Compile updated translation catalogs with Qt `lrelease`. New messages without
reviewed translations are explicitly unfinished and fall back to English;
do not mark them translated merely to make a completion counter increase.
`VirtualBox_en.ts` intentionally contains plural forms only. Before declaring
a locale fully supported, review the new translations and test long strings,
keyboard mnemonics and right-to-left layouts. The repository's `updatenls`
target remains the full catalog-maintenance entry point.

## Windows distribution checks

Build the normal installer targets in a release environment with the required
WiX tools and signing credentials. `NaveboxLauncher` and `VirtualBoxManifest`
are installation targets in the frontend makefile. A standalone developer
build of only selected executables must also build these targets before
packaging. Retain `VirtualBox.exe` and its file associations for compatibility;
the desktop/Start-menu entries and post-install launch use `Navebox.exe`.

On disposable clean hosts, verify:

1. Fresh install, first launch and VM creation as an ordinary user, without
   personal COM-registry repair scripts or SDK paths.
2. Both executable entry points, matching manifests, icons, taskbar grouping,
   pinned shortcuts and VM-file associations.
3. Upgrade from the previously supported package, repair and uninstall;
   check that old shortcuts and stale launcher copies do not remain.
4. Binary/package signatures and trust chain, file permissions and checksums.
5. Required Qt plugins, translations, runtime DLLs, documentation and Guest
   Additions availability. Exercise features that depend on those payloads.
6. VM/configuration compatibility when Oracle VirtualBox is present. Internal
   service and API names remain shared; do not advertise isolated side-by-side
   installations without testing installation ownership and upgrade behavior.

XML parsing and shortcut assertions do not replace WiX compilation, MSI
validation, Authenticode verification or clean-host installation tests.

For developer builds moved to another directory, Windows COM and VBoxSDS
registrations can still point to the old path. `REGDB_E_CLASSNOTREG` at startup
requires checking the effective 64-bit VirtualBoxClient `InprocServer32`, the
VirtualBox `LocalServer32` and the VBoxSDS service executable. Compare them with
the matching build directory before attempting repair. Close all managers and
VM processes, back up the affected registrations and repair with administrator
rights. A MIDL-proxy build registers its classes via the matching
`VBoxProxyStub.dll`; registering `VBoxC.dll` alone is insufficient. Do not run a
different installation's repair utility or overwrite the 32-bit registration
when repairing only a 64-bit developer build. The application does not change
these system registrations on startup.

## Native GUI and performance acceptance

- Repeatedly open and cancel Settings while navigating Storage, Network,
  System and menu-editor split buttons. Exercise UIAutomation and a real
  screen reader; check child indexing, stale providers and process lifetime.
- Verify all five mouse status states and all host-key capture/pressed states.
  Check light/dark/high-contrast palettes, selected rows and disabled controls.
- Change language and theme with About already open; keep the active tab and
  scroll position. Test keyboard operation and Copy Build Information.
- Test 100%, 125%, 150%, 175%, 200% and higher supported scaling, including
  moving windows between monitors. The preview frame no longer requires
  legacy bitmap resolutions.
- Measure cold startup, Settings load, event-loop p95/p99 latency, idle CPU,
  resident memory and repeated open/close behavior. Include realistic VM-list
  sizes and running-VM previews; record hardware, backend and build type.
- Optimize icon rasterization/cache size only after a representative profile.
  A fast cache microbenchmark is not an application performance budget.

## VMM, storage and recovery gates

The exit-history fix in `EMAll.cpp` must be validated independently of frontend
tests. Cover repeated successful probes with zero exit distance, valid
positive distances, success/failure return codes and action/continue-index
transitions. Exercise interpreter/recompiler and ring contexts where supported,
and every claimed NEM/HM backend. Do not publish exploit-like fixtures.

Run the relevant Runtime/Storage testcases and ValidationKit for the supported
scope. VM acceptance includes BIOS/EFI boot, SMP, pause/resume, saved-state
restore, snapshots, sustained disk/network workloads, shutdown and recovery
after process/host interruption. Check disk integrity and host/guest isolation.
Use disposable fixtures and known data; never use customer VMs as crash-test
fixtures. Record the exact coverage and any remaining exclusions.

## Release decision and operations

Do not promote a development folder or an unsigned local build to a qualified
distribution. Require a clean build, passing regression matrix, validated
signed package, dependency/license inventory, supported-locale review and
documented recovery path. Choose soak duration and performance limits from the
intended workload, then preserve the results in the release manifest.

Assign ownership for security updates, upstream patch intake, incident handling
and rollback. Keep the private disclosure route in `SECURITY.md` for upstream
security issues. Explain which update channel belongs to Navebox; do not
silently replace a fork installation with another product's package. Reuse
the existing log viewer and VBoxBugReport for diagnostics, allowing users to
review/redact exported data. No telemetry or automatic updater is enabled by
these frontend changes.
