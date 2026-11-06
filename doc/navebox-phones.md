<!-- $Id$ -->
<!-- Copyright (C) 2026 Oracle and/or its affiliates. SPDX-License-Identifier: GPL-3.0-only -->

# Android virtual phones in Navebox

Select **Phones** in the main navigation next to **Machines**. **File >
Android Phones** and the **Phones** button on the Home toolbar open the same page.
Android phones use Android Emulator, independently of VirtualBox VMs. The
emulator supplies touch input, rotation, volume, camera and other device
controls in its own window. VirtualBox VM files and their public API do not
change.

## Setup

Android Studio is optional. Install a standalone Android SDK with the
[Command-Line Tools](https://developer.android.com/tools/sdkmanager), or use
Android Studio's SDK Manager. Include Android
Emulator, Android SDK Platform-Tools, Android SDK Command-Line Tools (latest),
and a stable phone system image for API 26 or newer matching the host CPU.
The initial qualification target is Windows x64 with an x86_64 image. Other
host platforms require their own build and runtime qualification.

In Android Phones, click **SDK Settings** in the main toolbar, then select the SDK directory and
a compatible Java home. The Java home is the directory containing `bin/java`. It can be a compatible
standalone JDK, including a portable JDK, or Android Studio's bundled `jbr`.
Click **Apply SDK**, then **Check Acceleration**, then **Done**.
SDK Settings opens a modal centered over the manager. Applying an SDK keeps
setup open for the acceleration check. Click **Done** to close it.
Navebox uses installed SDK components and does not download packages, accept
licenses, enable Windows features or change firmware/security settings.
Setup instructions and acceleration requirements are maintained by Android:

- [Manage virtual devices](https://developer.android.com/studio/run/managing-avds)
- [Hardware acceleration](https://developer.android.com/studio/run/emulator-acceleration)

For installation without Android Studio, place the downloaded command-line
tools so the SDK contains `cmdline-tools/latest/bin` and
`cmdline-tools/latest/lib`, preserving the package contents. Use the SDK
package manager to install `emulator`, `platform-tools` and a stable phone
system image such as `system-images;android-35;google_apis;x86_64` for Windows
x64. Review and accept the provider's package licenses during your own setup.
Set `JAVA_HOME` to your compatible JDK or select that directory in Navebox.
Neither the Android Studio IDE, Gradle, NDK nor Android build-tools are needed
for this phone-management feature. The SDK directory can be on another drive.
Current Android documentation also offers an Android CLI for package setup;
this adapter still requires the command-line tools' AVD creation libraries.

On Windows, qualify the Windows Hypervisor Platform configuration alongside
the VirtualBox backend used by the installation. A successful acceleration
check does not qualify every GPU driver or Android system image.

## Create and use a phone

Click **Create Phone** in the main toolbar to open the creation dialog. Enter a name, choose an installed image, and set RAM,
CPUs, data-partition capacity and a portrait device profile. Defaults are 2048 MB,
up to two CPUs, an 8 GB data partition and 720 x 1280 pixels. Storage is a
maximum partition capacity and can consume additional host space as apps
and data accumulate. Leave enough RAM and disk space for the host and any
VirtualBox VMs running concurrently.

Choose **Vary for testing** to select from three stock SDK hardware profiles
with different portrait dimensions and densities. Navebox favors the least
used profiles in its catalog and picks randomly when their counts match. The
choice is saved at creation and remains stable when the phone restarts. For
repeatable tests, choose a specific resolution and density.

Each new phone has its own AVD identifier and guest data directory. Android
generates its app-scoped identifiers from the device and user state. Phones
using the same system image can share the same `Build.FINGERPRINT`, which
identifies the Android build. Use different installed Android images for build
coverage. The hardware profile option leaves the Android image and its build
properties intact.

Click **Create Phone** in the dialog. On success it closes and selects the new phone.
Each phone appears on the left with a phone icon, its name and its status.
Select a phone to see its general, system, display and storage details on the
right. **Start** in the toolbar is enabled for a selected, stopped phone with
a configured SDK. Click **Details** to inspect an operation's full output.
Booting and Ready are distinct states: **Install APK** and the navigation
actions under **More** are enabled after Android reports boot completion.
**More > Cold Boot** skips
loading a Quick Boot snapshot and preserves phone user data. **Stop** requests
shutdown of the selected emulator. Snapshot persistence is provided by the
installed emulator; no VirtualBox snapshot operations apply to these phones.
If GPU Auto fails, try **More > Cold Boot with Software Graphics**. This uses
the SDK's `-gpu software` mode for the session and skips loading or saving a
Quick Boot snapshot. The SDK must support that graphics mode. VM acceleration
and the writable Android data partition remain in use.

Switching to another page or closing Navebox leaves emulator windows running.
Return to **Phones** to refresh the list, or click **Refresh** at any time.
Close a phone's emulator window, or use **Stop**, to end its session.

With multiple phones, status refresh takes longer because each connected
emulator is checked separately. A phone action can interrupt a refresh and
uses the most recent completed status scan. Recheck **Ready** before running
an action if the connection has just changed. Concurrent capacity depends on
the host, image and emulator. Size the workload using measured RAM, disk,
CPU and GPU usage before starting a fleet.

Phone data is under `Phones` in Qt's application-local data directory.
**More > Open Log** opens the selected phone's emulator startup log. AVDs are kept in
the separate `Phones/avd` directory; Android Studio's AVD catalog is not
modified. SDK and Java paths are saved in `Phones/settings.json`. Display
names are independent of random AVD identifiers. Navebox only controls AVDs
with matching ownership metadata and a locator pointing into that private
catalog. It ignores physical devices and unrelated emulators, and verifies
the AVD identity again before each targeted operation.

## Troubleshooting and limits

If no image is listed, use the standalone SDK package manager or Android
Studio to install a stable `default`, `google_apis` or
`google_apis_playstore` phone image matching the host CPU, then click
**Apply SDK** again. Preview images, TV, Wear OS, automotive and test-device
images are outside this feature's initial scope. Image and Google-service
availability depend on the installed SDK packages.

If creation fails, read the diagnostic output. Confirm that Java is compatible
with the installed command-line tools and that the data directory is writable.
The SDK's `avdmanager-classpath.jar` and `AvdManagerCli` entry point are used
directly, avoiding Windows batch-shell interpolation. Future SDK tooling
changes must be qualified before replacing this integration. Failed creation
may leave an incomplete AVD directory; Navebox does not expose it as a usable
phone or overwrite existing phone data on retry.

If boot does not finish within five minutes, inspect **Open Log**, resolve the
reported SDK, image, hypervisor or GPU issue, and try **Cold Boot** after the
existing emulator has stopped. Boot timeout is diagnostic; it does not kill
the phone or erase data. The manager polls while it is visible, so a hung
guest cannot freeze the Navebox UI. SDK commands also have bounded execution
time and diagnostic output.

An APK requiring an unsupported native ABI, graphics capability or device
service can fail even when Android itself boots. Installation failures are
reported with the SDK diagnostic. This feature does not guarantee that every
APK, game, banking app or device-attestation flow works. It does not create
an iOS device, a real cellular subscription, or real SMS/calling service.

## Validation and release gate

Build `UICommon`, `Navebox`, `NaveboxManifest` on Windows, `tstNaveboxPhone`
and `tstNaveboxGui` in the configured developer environment with
`VBOX_WITH_TESTCASES=1`. Explicitly install all frontend targets in a narrow
build: an executable combined with an older `UICommon` can fail at startup.
Run the two test executables
from the matching output tree with the installed Qt libraries/plugins on the
loader path. `tstNaveboxPhone` uses an isolated executable mock SDK: it tests
subprocess argument preservation, Unicode paths, input validation, creation,
boot-state transitions, ADB isolation, connection changes, APK failure,
acceleration failure, output limits, persistence and the dialog. These checks
do not boot Android or establish real image/hypervisor compatibility.

Before shipping, perform these checks on a clean supported Windows x64 host:

1. Use **Check Acceleration**, create a phone from the chosen release image,
   boot to Ready and verify touch, keyboard, portrait/landscape and networking.
2. Install an ABI-compatible test APK and verify Back, Home and Recent Apps.
   Confirm that an incompatible or corrupt APK produces a useful error.
3. Stop and restart the phone. Confirm persisted user data, Quick Boot and
   Cold Boot; close/reopen Navebox while the emulator remains running.
4. Run two distinct phones with a physical ADB device and an Android Studio
   emulator connected. Confirm every action affects only the selected phone.
5. Test paths containing spaces, Unicode and shell metacharacters, a missing
   SDK/image/Java installation, failed acceleration, low disk space, an
   emulator startup failure and an unresponsive guest.
6. Repeat with the shipped VirtualBox execution backend and a running ordinary
   VM. Check installed-package operation, native accessibility, light/dark
   themes, high DPI and screen sizes. Record emulator/image/tool versions,
   Windows version, CPU, GPU/driver and backend results.

Do not label a build runtime-qualified until these real Android checks pass.

## Checks performed in this checkout (2026-11-03)

The Windows x64 release build completed for `UICommon`, `VirtualBox`,
`NaveboxManifest`, `tstNaveboxPhone` and `tstNaveboxGui`. The installed
`UICommon.dll` and both executable entry points were checked against the
new build by SHA-256. The main manager reached `showEvent` under an
offscreen debugger smoke check after all frontend binaries were synchronized.

All seven phone regression groups passed, including real mock subprocesses,
timeout/output limits and styled field geometry. The existing GUI regression
executable, six packaging tests and four build-helper tests passed. Both phone
tabs were rendered and reviewed. XML parsing and whitespace checks passed.
SCM checked copies of the new C++ files with SVN-only actions disabled because
this environment has no SVN executable or working copy. The existing Qt SDK's
`Qt6EntryPoint.lib` produces linker warning LNK4291 about conservative exception
continuation metadata; no third-party files or compiler warning policy were
changed for this feature.

For a real boot check, standalone Google command-line tools (archive
`commandlinetools-win-15859902_latest.zip`), Android Emulator 37.2.13
(build 16740729), Platform-Tools, the Android 15/API 35 Google APIs x86_64 image
(revision 9), and Temurin JDK 21.0.13+9 were installed under the ignored local
directory `prebuild/android-phone-runtime`. Downloaded command-line tools and
JDK archives were checked against their providers' SHA-256 digests. Android
Studio was not installed. The acceleration check reported WHPX as usable.

The production backend created **Navebox Android 15** in its real private
catalog and observed Ready after 31 seconds. ADB confirmed Android 15,
API 35, boot completion, a 720 x 1280 display and density 320. The backend's
Home action succeeded and an ADB screen capture showed the Android launcher.
The emulator remains a separate running application. The requested 2048 MB
configuration was raised to 2560 MB by this emulator/image combination.

This host needed app-local hard links to the downloaded emulator's own DLLs
beside its QEMU executables to resolve startup dependencies. The local SDK
was adjusted for this check; Navebox does not perform this workaround or
change an arbitrary user's SDK. A clean unmodified SDK installation therefore
still needs separate qualification. No system DLLs or Windows settings were
changed.

APK compatibility, snapshot persistence, sustained performance, GPU coverage
and coexistence with a running VirtualBox VM remain unverified. Native desktop
automation returned `GetCursorPos: Access is denied`, so dialog layout checks
used offscreen rendering. The initial additional render using the real catalog
was delayed by an automatic approval reviewer usage-limit error; it completed
on 2026-11-04 using the selected SDK and actual catalog.
These results do not qualify an installer for release.
No public API, `.xidl`, host driver or VirtualBox VM format was changed.

The 2026-11-04 layout revision uses the same card treatment for SDK setup and
creation, keeps setup separate from the empty catalog, and normalizes control
geometry independently of the application's shared stylesheet. Regression
coverage includes equal field/action heights, matching action widths, aligned
SDK/Java rows, and keeping Check Acceleration available after applying an SDK.
All seven phone regression groups passed with Windows style without the
application stylesheet; the existing GUI regression also passed. The real
catalog render showed the user's new **phone 1** as Ready and the earlier
**Navebox Android 15** phone as Stopped. It did not start or stop either phone.
After closing the running manager with the user's approval, all frontend
targets were built and staged successfully. SHA-256 checks confirmed that
both `VirtualBox.exe` and `Navebox.exe` matched the linked frontend and that
`UICommon.dll` matched its build output. A native Windows smoke check reopened
the updated Navebox, opened Android Phones, and confirmed **phone 1** was still
Ready with the saved SDK configuration. The Android emulator was left running
throughout the manager restart.

## Multi-phone checks (2026-11-06)

An isolated catalog on Windows x64 (build 26300.9711) used Android Emulator
37.2.13, Platform-Tools 37.0.1, WHPX, and the API 35 Google APIs x86_64 image
revision 9. The SDK still had the local emulator DLL workaround documented
above. Eight consecutive creates took about 0.7–0.9 seconds each. Each had a
different AVD ID and data directory. Automatic profile allocation distributed
the eight devices across the three supported definitions with counts 3/3/2.

Two phones cold-booted concurrently with Software Graphics and reached Ready
in 54–57 seconds. One full status refresh took 234 ms. ADB observed different
resolutions (1080 x 1920 and 1080 x 2160), distinct shell-scoped Android IDs and
independent writable guest data. The Android build fingerprint, reported model,
brand and device were equal because both used the same system image. The
backend installed the image's stock Android Easter Egg APK on one fixture
without affecting the other, reported a malformed APK, and stopped each phone
individually. Eight created devices and two running devices are distinct test
limits.

One GPU Auto run lost an emulator during concurrent boot. Windows attributed
the fault to the NVIDIA OpenGL driver (`nvoglv64.dll`). A repeat with GPU Auto
passed. Software Graphics passed this two-device workload. This does not
qualify long-running workloads, larger concurrent fleets, snapshots, other
images/hosts, or coexistence with a running VirtualBox VM.
