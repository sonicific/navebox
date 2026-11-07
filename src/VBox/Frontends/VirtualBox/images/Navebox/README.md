# Navebox identity

The editable artwork is `navebox.svg`: a cyan/cobalt folded N with a white
ribbon on a pale rounded plate. Semantic action/status icons use the same blue
family while retaining green, amber and red for their existing meanings.
The PNG, ICO and ICNS files are exports
of that SVG; do not retouch individual resolutions. The ICO contains 16, 20,
24, 32, 40, 48, 64, 128 and 256 pixel images. The ICNS contains 128 through
1024 pixel images. There is no bitmap shadow outside the rounded silhouette.

Regenerate the assets with `scripts/render-navebox-branding.cpp`, a developer
utility linked against Qt Core, Gui and Svg. This does not add Qt Svg to the
application's dependencies. In a configured MSVC developer shell, compile it
with `/std:c++17 /Zc:__cplusplus /permissive- /EHsc /MD`, the Qt include paths,
and `Qt6Core.lib Qt6Gui.lib Qt6Svg.lib`. Then run:

```text
render-navebox-branding.exe images/Navebox/navebox.svg images/Navebox
```

The legacy BIOS uses a separate compiled bitmap, selected by
`VBOX_BRAND_BIOS_LOGO` in both OSE and non-OSE configurations. Regenerate it from
the same icon SVG and the geometric, font-independent boot wordmark:

```text
render-navebox-branding.exe --boot images/Navebox/navebox.svg ../../Artwork/Navebox/boot-wordmark.svg ../../Artwork/Navebox/navebox-bios.bmp
```

Paths above are relative to the frontend directory. The output is a 560x160,
uncompressed, bottom-up 24-bit BMP on black, centered by the BIOS with its F12
boot-menu prompt retained. Rebuild `VBoxDD` after changing this asset; changing
the desktop icon alone does not change the guest boot display. Custom per-VM
boot images retain priority. EFI logos belong to the separate EFI firmware;
updating this BIOS bitmap does not modify prebuilt EFI images.

Windows setup artwork also uses the same icon and wordmark. With the Qt JPEG
image plugin available to the renderer, export all three installer backgrounds:

```text
render-navebox-branding.exe --installer images/Navebox/navebox.svg ../../Artwork/Navebox/boot-wordmark.svg ../../Artwork/Navebox
```

The dialog is 493x313, the banner 493x58 and the EXE splash 800x200 pixels.
The corresponding `VBOX_BRAND_WIN_INST_*` variables select these files; their
build dependencies force resources to update when artwork changes. MSI product
name comes from `VBOX_PRODUCT`, and distributor display from
`VBOX_BRAND_WIN_INST_PUBLISHER`. Original license/copyright text is retained.

Product name, slogan and contact data are centralized in
`src/globals/UIBranding.h`. Windows builds install `Navebox.exe` as the
manager and `NaveboxVM.exe` as the VM frontend. The manager's visual manifest
uses the branded 70 and 150 pixel images. Upstream attribution is available
in About.

The same utility creates four small chevrons for flat dropdown and spinbox
buttons. Light and dark variants retain a visible arrow after native button
bevels are removed. Their geometry is defined in the renderer source.

Windows startup requests the `Navebox.Desktop` taskbar identity through
[SetCurrentProcessExplicitAppUserModelID](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-setcurrentprocessexplicitappusermodelid),
resolved through IPRT's system-library loader before creating any windows.
This also works when the launching process has not loaded Shell32, so a child
process does not inherit the launcher's taskbar group.
