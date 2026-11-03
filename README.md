<div align="center">
  <h1>
    <img src="src/VBox/Frontends/VirtualBox/images/Navebox/navebox.svg" alt="Navebox icon" title="Navebox" width="48" valign="middle">
    Navebox
  </h1>
  <p><strong>Command Your Virtual World</strong></p>
  <p>A branded desktop frontend for VirtualBox.</p>

  <a href="https://github.com/sonicific/navebox/actions/workflows/navebox-checks.yml">
    <img src="https://github.com/sonicific/navebox/actions/workflows/navebox-checks.yml/badge.svg" alt="Navebox integration checks">
  </a>
  <a href="https://github.com/sonicific/navebox/blob/main/COPYING">
    <img src="https://img.shields.io/badge/license-GPL--3.0-blue.svg" alt="GPL-3.0 license">
  </a>
  <a href="https://www.virtualbox.org/">
    <img src="https://img.shields.io/badge/based%20on-Oracle%20VirtualBox-183A61.svg" alt="Based on Oracle VirtualBox">
  </a>
</div>

## 📖 About

Navebox is a desktop frontend built from the VirtualBox source tree. It gives
the graphical manager a distinct Navebox identity while retaining the
underlying VirtualBox API, service and VM-file identifiers for compatibility.
It is intended for development, experimentation and controlled distribution;
a local development build is not automatically a qualified release.

The project is currently focused on the Windows desktop frontend. The
underlying VirtualBox tree supports additional host platforms, but Navebox
branding and packaging should be considered supported only where they have
been built and tested.

## ✨ Features

- 🧭 Navebox desktop identity, logo, icons and taskbar grouping.
- 🖥️ Create, configure, start, pause and manage virtual machines through the
  familiar VirtualBox manager.
- 📱 Create and manage Android virtual phones through **File > Android
  Phones**, using a separately installed Android SDK. See the
  [phone setup and validation guide](doc/navebox-phones.md).
- 💾 Support for VirtualBox VM files and internal service/API compatibility.
- 🎨 Light, dark and high-DPI-aware interface behavior.
- 🌍 Existing VirtualBox translation and accessibility infrastructure.
- 🔧 A focused regression suite for branding, packaging, dialogs and GUI state.

## 📊 Project status

This repository is a development checkout, not a prebuilt binary download.
Build provenance, supported backends, packaging requirements and release gates
are documented in the [Navebox release and validation guide](doc/navebox-release.md).

Do not copy `Navebox.exe` by itself and treat it as a portable application.
Normal deployment requires the matching VirtualBox frontend/backend binaries,
COM registration, VBoxSDS service, Qt runtime/plugins and the feature payloads
used by the selected installation. Prefer a complete, tested installer for
end-user deployment.

## 🚀 Getting started

### 🧰 Requirements

Building VirtualBox requires a platform-specific compiler, SDKs, Qt and other
dependencies. Follow the official [VirtualBox build
instructions](https://www.virtualbox.org/wiki/Build_instructions) before
configuring the tree.

Use the repository's external developer setup:

```text
configure.py
```

Review the generated configuration and keep machine-local settings in
`LocalConfig.kmk`. Internal developers should use the environment scripts
described in the [release and validation guide](doc/navebox-release.md).
Prefer `KBUILD_*` variables and keep release builds in a separate output tree.

### 🔨 Build

In a prepared developer shell, build the tree with:

```text
kmk
```

For Windows x64, the repository includes a build entry point. After preparing
the local configuration, run:

```text
.\build.cmd doctor
.\build.cmd check
.\build.cmd release
```

`release` compiles, stages Qt, builds MSI/EXE, verifies the embedded artwork and
payload, then writes installers, a manifest and SHA256 hashes to
`out/win.amd64/release/delivery`. Run `build.cmd --help` for individual stages.
It does not install the application or change Windows security settings.

For a new checkout, copy `navebox-build.example.json` to
`navebox-build.local.json`, prepare `LocalConfig.release.kmk` using
`tools/navebox/LocalConfig.release.example.kmk`, and configure the actual SDK
paths and signing inputs. Both local files are ignored. The example is a
host-only profile; compiler/SDK compatibility fixes remain machine-local.
See the [build guide](doc/navebox-release.md#windows-build-entry-point) for
requirements and commands. These builds still need signing, clean-host
installation tests and hypervisor qualification before public release.

## 🧪 Testing

Run the dependency-free packaging and catalog checks from the repository root:

```text
python src/VBox/Frontends/VirtualBox/testcase/tstNaveboxPackaging.py
```

Build and run the GUI regression executable in a configured environment:

```text
kmk -C src/VBox/Frontends/VirtualBox VBOX_WITH_TESTCASES=1 tstNaveboxGui
out/<host>.<arch>/<build-type>/bin/testcase/tstNaveboxGui
```

On Windows, append `.exe` to the test executable. The test documentation
describes Qt offscreen execution, snapshot export and native-platform checks:
[frontend regression checks](src/VBox/Frontends/VirtualBox/testcase/README.md).
These tests do not replace native Windows accessibility, installer,
driver-signing or VM compatibility validation.

## 🎨 Branding and icons

The editable Navebox artwork is
[`navebox.svg`](src/VBox/Frontends/VirtualBox/images/Navebox/navebox.svg).
The repository also contains generated PNG and ICO sizes used by the
application and installer. Do not edit individual exported resolutions.

Branding assets can be regenerated with the Qt-based
[`render-navebox-branding.cpp`](src/VBox/Frontends/VirtualBox/scripts/render-navebox-branding.cpp)
utility. See the [asset instructions](src/VBox/Frontends/VirtualBox/images/Navebox/README.md)
for the required Qt libraries and command.

## 📚 Documentation

- [Release and validation guide](doc/navebox-release.md) - build provenance,
  Windows deployment, execution backends and acceptance gates.
- [Frontend regression checks](src/VBox/Frontends/VirtualBox/testcase/README.md)
  - automated packaging and GUI test instructions.
- [VirtualBox User Guide](https://docs.oracle.com/en/virtualization/virtualbox/index.html)
  - user-facing VM configuration and product documentation.
- [VirtualBox technical documentation](https://www.virtualbox.org/wiki/Technical_documentation)
  - architecture and developer references.

## 🤝 Contributing

Bug reports and feature requests are welcome through the
[issue tracker](https://github.com/sonicific/navebox/issues). Before opening an
issue, check whether it is a security vulnerability; security issues must
follow the [security policy](SECURITY.md) and must not be reported publicly.

For code contributions, read [CONTRIBUTING.md](CONTRIBUTING.md), keep changes
focused, update related documentation and include the narrowest useful
validation evidence. Do not commit generated build outputs, local
`LocalConfig.kmk` settings, VM images, logs or credentials.

## 🔒 Security

Treat guest-controlled, network-controlled, disk-controlled and user-supplied
input as untrusted. Report suspected vulnerabilities privately according to
the [security policy](SECURITY.md) and the upstream
[vulnerability reporting guide](https://www.oracle.com/corporate/security-practices/assurance/vulnerability/reporting.html).

## ⚖️ License

Navebox follows the licensing terms of the VirtualBox source tree. The main
program is released under the [GNU General Public License v3.0](COPYING).
Additional components may use other licenses; see
[COPYING.CDDL](COPYING.CDDL), [COPYING.LIB](COPYING.LIB) and
[THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt).

Copyright notices and third-party license terms remain applicable to the
corresponding source files and distributed components.
