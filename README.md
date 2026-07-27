# Discover for Fluff Linux

Discover is the Fluff Linux application manager, based on KDE Discover. It
provides a native Plasma 6 experience for finding, installing, removing, and
updating applications.

Version `26.8.1` is intentionally focused on Flatpak:

- Flatpak is the only application backend built by this repository.
- App updates are provided only through Flatpak.
- System package updates are handled by Fluff Linux Update.
- Firmware updates are not part of Discover.
- PackageKit, Snap, KNewStuff, rpm-ostree, systemd-sysupdate, Alpine APK, and
  other software backends are not included.

The interface follows the Plasma theme, uses Fluff Linux branding, and supports
right-to-left layouts.

## Build on Fluff Linux or Arch Linux

Install the build requirements:

```sh
sudo pacman -S --needed base-devel cmake extra-cmake-modules ninja \
    appstream-qt discount flatpak \
    karchive kcmutils kconfig kcoreaddons kcrash kdbusaddons ki18n \
    kiconthemes kidletime kio kirigami kirigami-addons kjobwidgets \
    knotifications kservice kstatusnotifieritem \
    kwidgetsaddons kwindowsystem purpose qcoro qqc2-desktop-style \
    qt6-base qt6-declarative qt6-webview vulkan-headers
```

Configure and compile:

```sh
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DKDE_INSTALL_LIBDIR=lib \
    -DBUILD_TESTING=OFF
cmake --build build
```

Flatpak and AppStream are required. Configuration stops with an error if the
Flatpak development files are unavailable.

If an older copy was built in the same directory, remove `build/` first. This
is especially important after changing backend or translation resources.

## Stage for packaging

Fluff Linux packages are assembled from a repository-local fake install root
instead of being installed directly onto the build machine:

```sh
# fakeroot/ becomes the package filesystem and can be passed to the Fluff Linux packaging tools.
DESTDIR="$PWD/fakeroot/" cmake --install build
```

Everything that belongs in the package will be placed under:

```text
fakeroot/
├── etc/
└── usr/
```

The exact directories present depend on the enabled components. This staging
directory is only a packaging workspace; installing into it does not modify the
host system.

Use an empty staging directory for every package build so files removed by a
newer version cannot remain in the finished package.

## Verify the Flatpak-only build

The staged backend directory should contain the Flatpak backend and no firmware,
PackageKit, or Snap backend:

```sh
find "$PWD/fakeroot/usr" \
    \( -iname '*flatpak*' -o -iname '*fwupd*' -o -iname '*packagekit*' -o -iname '*snap*' \) \
    -print
```

The output may include Flatpak files only. Any fwupd, PackageKit, or Snap file
indicates that the staging directory was not cleaned before installation.

The package must use `fakeroot/usr/lib`, not `fakeroot/usr/lib64`.
`/usr/lib64` is owned by `flufflinux-filesystem` and must not be included in
this package.

## License

Fluff Linux Discover retains KDE Discover's upstream licensing. Each source
file remains governed by its SPDX license identifier; the repository includes
both GPL and LGPL components. Complete license texts are available in
[`LICENSES/`](LICENSES/).

The Fluff Linux package metadata identifies the package as
`LGPL-2.0-or-later`, matching the official Arch Linux Discover package.

## Test locally

The application can be launched from the build tree:

```sh
./build/bin/plasma-discover
```

To test the staged files as a system installation, use a disposable development
system and install the build:

```sh
sudo cmake --install build
kbuildsycoca6
plasma-discover
```

Do not install development builds directly on production systems.

## Translations

Translations are stored below `po/<language>/` and installed automatically by
the packaging step. The primary catalogs are:

- `plasma-discover.po` for the application interface.
- `plasma-discover-notifier.po` for taskbar notifications.
- `libdiscover.po` for shared application-management text.
- `kcm_updates.po` for the app-update controls integrated into Discover Settings.

To verify the staged catalogs:

```sh
find "$PWD/fakeroot/usr/share/locale" \
    -path '*/LC_MESSAGES/*.mo' -print
```

## Upstream

This project is derived from [KDE Discover](https://invent.kde.org/plasma/discover).
The upstream release history and copyright notices are preserved.

## License

Discover is available under the license terms recorded in
[`LICENSES/`](LICENSES/).
