# QuestLHSync

Lighthouse trackers, controllers and base stations aligned to a Quest's or a
Steam Frame's own tracking, with nothing mounted on the headset. The headset's
tracking cameras see the laser flashes of SteamVR base stations, and
QuestLHSync uses them to keep the lighthouse space lined up with the
headset's while you play. No SpaceCalibrator, no tracker strapped to your head.

It has two parts: a **headset part**, which serves what the cameras see on your
local network (a Magisk module on a Quest, a user service and a small SteamVR
driver on a Steam Frame), and a **SteamVR driver** for the PC, which solves the
alignment, applies it to every lighthouse device and adds a page to the SteamVR
dashboard.

Steam Frame and Linux support is by [@NotZoeyDev](https://github.com/NotZoeyDev).

Galaxy XR support is by [@stearnsbq](https://github.com/stearnsbq), with the
[questlhsync-galaxy-xr](https://github.com/stearnsbq/questlhsync-galaxy-xr)
headset module.

![The QuestLHSync dashboard page](docs/dashboard.png)

## Requirements

- A **Quest Pro, Quest 3 or Quest 3S**, rooted with
  [Magisk](https://github.com/topjohnwu/Magisk).
- Or a **Steam Frame**. No root, no developer password: everything installs as
  your user.
- **SteamVR base stations**, 1.0 or 2.0.
- **Windows with SteamVR**, with the headset streamed by anything: Steam Link,
  Link, Air Link, Virtual Desktop, ALVR, CreoleCast...
- Or **Linux with SteamVR or WiVRn**, with the headset streamed by anything that
  works on Linux (Steam Link, WiVRn...).
- The PC and the headset on the **same local network**.
- At least **one lighthouse device switched on** (a tracker or an Index
  controller). SteamVR only shows base stations while one is on.

## Install

Each device has its own file in
[Releases](https://github.com/CreoleVR/QuestLHSync/releases).

1. **Headset:**
   - **Quest:** install `QuestLHSync-quest-module-<version>.zip` in the Magisk
     app (**Modules > Install from storage**) and reboot. Or over adb:

     ```
     adb push QuestLHSync-quest-module-<version>.zip /sdcard/Download/
     adb shell su -c "magisk --install-module /sdcard/Download/QuestLHSync-quest-module-<version>.zip"
     adb reboot
     ```

   - **Steam Frame:** in Desktop Mode, open `QuestLHSync-frame-installer.flatpak`
     in Discover, then start **QuestLHSync Installer** and click **Install**. Or
     copy `QuestLHSync-frame-module-<version>.tar.gz` to the Frame and, in a
     terminal on it or over ssh:

     ```
     tar xzf QuestLHSync-frame-module-<version>.tar.gz
     QuestLHSync-frame/install.sh
     ```

     Either way, SteamVR on the headset restarts once to load its driver.

2. **PC:** run `QuestLHSync-steamvr-installer.exe`. It downloads the latest
   release, installs the driver to `%LOCALAPPDATA%\QuestLHSync\questlhsync`,
   registers it with SteamVR (in place of a copy you registered by hand) and
   offers an update whenever a newer release is out. It closes SteamVR if it's
   running.

   Or by hand: extract the `questlhsync` folder from
   `QuestLHSync-steamvr-<version>.zip` somewhere permanent and, with SteamVR
   closed, register it:

   ```
   "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" adddriver "C:\path\to\questlhsync"
   ```

3. Turn off SpaceCalibrator, OpenVR-SpaceSync or anything else that moves
   lighthouse devices. Two tools correcting the same devices fight each other.

### Linux

Pick by what runs your VR:

- **WiVRn or Monado:** run `QuestLHSync-wivrn.AppImage` from the
  [Releases](https://github.com/CreoleVR/QuestLHSync/releases). That is all you need on the PC: no installer, no
  SteamVR driver. See [WiVRn / Monado](#wivrn--monado-linux) below.
- **SteamVR:** run `QuestLHSync-steamvr-installer.AppImage`. It is strictly for SteamVR users: it installs the
  SteamVR driver and registers it with SteamVR, which WiVRn and Monado don't use.

## Use

Start SteamVR as usual. QuestLHSync finds the headset by itself. Its page is in
the SteamVR dashboard, with a copy on the desktop.

The first time, look around so the cameras catch both base stations (the
Frame uses only its two upper cameras: the lower pair looks at the floor, not
at the base stations). It locks within about half a minute,
and after that starts from the saved alignment. Wear a tracker or hold a
controller meanwhile: when two base stations could be either way round, the
devices you wear or hold decide.

With three base stations, glance at the third one too. QuestLHSync's reference
frame keeps whatever tilt SteamVR's lighthouse space had when it was first
seen, and with two base stations in view a tilt of 1.5° puts trackers on the
floor 15 cm to the side. Once the cameras have seen three base stations well
enough to tell, QuestLHSync levels the reference frame with the headset's
gravity and keeps the level in `stations.json`. With two, the lighthouse
controllers' and trackers' accelerometers level it as they move about.

- **Pause corrections** holds lighthouse devices where they are.
- **Record session** saves what the driver receives to a file, for bug reports.
- **Force recalibrate** drops everything learned about the alignment and starts
  over, for when it sits stuck on a wrong fit. It clears the sightings, the fit,
  the headset's saved alignment and pose timing, and an automatic reference frame
  (`stations.json`: pinned again from SteamVR's base stations, its level found
  again). Lighthouse devices go back to SteamVR's own places until the cameras
  have found the base stations again. A reference frame you measured yourself
  stays.

What a headset's cameras have seen is kept per headset, in
`%LOCALAPPDATA%\QuestLHSync\headsets\<model>-<id>\state.json` (the id is a hash
of its serial number), so switching between headsets doesn't seed one with the
other's alignment or pose timing. The `state.json` of earlier versions moves to
the first headset that connects. `stations.json`, the reference frame of your
base stations, is shared by all headsets.

## Settings

Optional, in `steamvr.vrsettings` under `"driver_questlhsync"`:

| Key | Default | |
|---|---|---|
| `enable` | `true` | `false` turns QuestLHSync off |
| `host` | `""` | The headset's IP address(es), comma-separated, for networks that drop broadcasts |
| `headset` | `""` | A headset serial to prefer when several answer |
| `anyHmd` | `false` | Use SteamVR's headset even when it isn't named a Quest Pro, 3, 3S or Steam Frame |
| `record` | `false` | Record every session (same as the button) |
| `gravity` | `true` | `false` turns levelling by the lighthouse devices' accelerometers off |
| `steadyStations` | `true` | `false` lets SteamVR move base stations by each new measurement instead of their average |

With a wired headset (SteamVR's headset is a lighthouse one, or tracked by a driver that isn't a streamer's: Index, Vive, Windows Mixed Reality, ...), QuestLHSync switches itself off: no corrections, no levelling, no base station averaging, and the dashboard closes. `anyHmd` keeps it on for a headset it wouldn't otherwise accept (never a lighthouse one).

## Troubleshooting

- **"Looking for the headset":** the headset must be awake and on the same
  network. If your router drops broadcasts, set `host`.
- **"No camera frames":** the cameras only run while the headset tracks. Put it
  on. If it stays there, check `%LOCALAPPDATA%\QuestLHSync\questlhsync.log`:
  a `no camera buffers found` line means the headset part doesn't know this
  headset or OS build yet. Please open an issue with that line. On the Frame, a
  "questlhsync_frame SteamVR driver isn't running" line means SteamVR on the
  headset hasn't restarted since the install:
  `systemctl --user restart steamvr.service`.
- **"Waiting for base stations":** switch on a tracker or controller.
- **"Finding the base stations" for a long time:** face each base station for a
  few seconds.

## Privacy and security

The PC side writes only to `%LOCALAPPDATA%\QuestLHSync` and only talks to the
headset. The headset part answers anyone on the local network without
authentication. It sends bright-spot positions (never images), the camera
calibration, and the headset's serial number, model and firmware. Use it on a
network you trust. It only reads the cameras while a PC is connected, and
patches nothing on disk. On the Frame, `lhsyncd` logs to the user journal
(`journalctl --user -u questlhsync`).

## WiVRn / Monado (Linux)

WiVRn and Monado have no SteamVR host to hook, so `questlhsync-xr` is a desktop app that does the same job from
outside: it shows the same dashboard page in a window, uses the same headset link and solver, reads the headset's and
the lighthouse devices' poses over OpenXR (`XR_MNDX_xdev_space`, a headless session, as `motoc` does), including live
base station poses. It applies the result as the Lighthouse tracking origin offset through libmonado. It needs
WiVRn or Monado 25.0 or newer **with the base-station xdev patch**, SteamVR tracked devices enabled (not the WiVRn
Flatpak, which can't use the lighthouse driver), SteamVR installed (not required to be running), and the OpenXR loader.
Both the loader and libmonado are found at runtime: libmonado from the active runtime manifest's `MND_libmonado_path`.
`--runtime FILE` takes precedence over `XR_RUNTIME_JSON`. With neither an explicit runtime nor an active runtime
registration, a source build in this workspace also looks for the sibling
`WiVRn/build-lh-sync/openxr_wivrn-dev.json`. This uses WiVRn's existing build manifest without generating another
one or changing system settings. For other layouts, pass that manifest explicitly:

```sh
./QuestLHSync/out/questlhsync-xr --runtime "$PWD/WiVRn/build-lh-sync/openxr_wivrn-dev.json" --dump
```

The patched `steamvr_lh` must expose stations with their original `LHB-...` serials and
`XRT_INPUT_GENERIC_TRACKER_POSE` in the same tracking origin as the `LHR-...` devices. With the companion WiVRn
patch this is on by default; do not set `LH_EXPOSE_BASE_STATIONS=0`. Use the companion xrizer patch to exclude
these stations from game-facing body trackers. The headset's existing QuestLHSync module is unchanged.

Station positions and orientations come only from OpenXR. `lighthousedb.json` is optional and supplies rotor
channels for identity checks; without it those channel checks are unavailable. The app reads STAGE and Lighthouse
origin offsets to recover uncorrected station/tracker poses, and writes only the Lighthouse origin. User reference
space offsets are left under the user's control. If offset reads fail or change during a pose batch, that batch is
skipped. Stations in different origins, or an HMD sharing the Lighthouse origin, hold corrections rather than mixing
unrelated frames or moving the HMD.

`./build.sh --wivrn` builds `out/questlhsync-xr` and packs it, SDL2 and a font into
`out/QuestLHSync-wivrn.AppImage`. Start the app, then connect the headset: the app waits for it. Device and origin
snapshots refresh once a second, including devices added after connection when the runtime supports hotplug.
The page's buttons pause the corrections and
record a session. `--host IP` and `--headset SERIAL` as in the driver's settings; `--no-window` runs without the
window; `--dump` lists the devices, their tracking origins and raw `LHB-` poses without solving or applying corrections.
If no stations appear, check that the runtime includes the base-station patch. State and the log are in
`~/.local/share/QuestLHSync` (`questlhsync-xr.log`). No base station averaging or gravity levelling yet.

`sh scripts/test_xr.sh` runs hardware-free tests of coordinate conversion, live station discovery, tracking loss,
reconnect offset application and reference-space changes. It requires a C++17 compiler, SDL2 development files and
pkg-config. Real camera timing and calibration accuracy still require an HMD and base stations.

## Uninstall

- **Quest:** remove the module in the Magisk app and reboot.
- **Steam Frame:** click **Uninstall** in QuestLHSync Installer, or run
  `QuestLHSync-frame/uninstall.sh` on the headset.
- **PC:** with SteamVR closed, click **Uninstall** in the installer, or run
  `vrpathreg removedriver "C:\path\to\questlhsync"` and delete
  `%LOCALAPPDATA%\QuestLHSync`.

## Building

Visual Studio 2022 with C++, the Android NDK, [Zig](https://ziglang.org) and
Python 3:

```
build.bat                        SteamVR driver + dashboard app (SteamVR closed) + out\QuestLHSync-Installer.exe
build.bat installer              only the installer
python magisk\build_module.py    Quest module (Magisk), into out\
python frame\build.py            Steam Frame package, into out\
python release.py                the release files, into out\release-<version>\
python install.py                register driver\questlhsync with SteamVR
python install.py headset        install the module over adb, no reboot
```

On Linux (x86-64) everything builds through `./build.sh`, which runs the scripts in `scripts/`:

```
./build.sh                  the SteamVR driver and its dashboard app (same as --driver)
./build.sh --installer      out/QuestLHSync-steamvr-installer.AppImage (builds the driver first)
./build.sh --wivrn          out/questlhsync-xr and out/QuestLHSync-wivrn.AppImage
./build.sh --all            the three above, with the driver built once (flags combine: ./build.sh --installer --wivrn)
./build.sh --frame          the Steam Frame's driver and installer (run it on the Steam Frame, see below)
```

For SteamVR (in Valve's sniper container), `./build.sh` builds the driver
(`driver/questlhsync/bin/linux64/driver_questlhsync.so`) and the dashboard app (`QuestLHSync`, with
`libopenvr_api.so`, next to it) in Valve's sniper SDK image with docker or podman (`QLHS_HOST_BUILD=1` uses the
host compiler instead). With SteamVR closed, `python3 install.py` registers `driver/questlhsync` with SteamVR, and
`"activateMultipleDrivers": true` must be set under `"steamvr"` in `steamvr.vrsettings`. The data folder is
`~/.local/share/QuestLHSync`. The driver starts the dashboard app, which draws the page in software (it needs a
Liberation, DejaVu or Noto Sans font on the system); it also shows the page in a desktop window (SDL2). 
Gravity levelling reads the lighthouse receivers' hidraw nodes, which needs Valve's udev rules (Steam installs them).

`./build.sh --installer` builds `out/QuestLHSync-steamvr-installer.AppImage` (builds the driver first). It
runs on any x86-64 distro with glibc 2.31+, FUSE (or `--appimage-extract-and-run`), libcurl and a display, and
installs to `~/.local/share/QuestLHSync/questlhsync`. Like the Windows installer it downloads the latest release's
`QuestLHSync-linux-module-<version>.zip` (`release.py` builds it from `./build.sh`'s output) and offers an update
when a newer release is out. It also carries the driver, the dashboard app, SDL2 and a font, and installs those when
GitHub isn't reachable. Installing stops SteamVR, replaces the driver folder, registers it with `vrpathreg`,
sets `activateMultipleDrivers`, and unregisters other QuestLHSync copies (a source checkout).
Without a display, `--install`, `--uninstall` and `--status` do the same from a terminal.

`./build.sh --frame` builds the Steam Frame's package (`out/QuestLHSync-frame-<version>.tar.gz`: `lhsyncd` and the
`questlhsync_frame` SteamVR driver, by `frame/build.py`) and its installer app
(`out/QuestLHSync-frame-installer-<version>.flatpak`, by `frame/installer/build.py`). **Run it on the Steam Frame
itself, or another arm64 Linux:** both build for the machine they run on, so `--frame` refuses to run anywhere else
(it isn't part of `--all`). The Frame's own `cc` and `c++` are used; the Flatpak needs flatpak-builder and the GNOME
SDK: `flatpak install --user flathub org.flatpak.Builder org.gnome.Sdk//50`. From another machine, Zig can
cross-compile the package alone (`python3 frame/build.py`, with `zig` on PATH or `ZIG` set to it); the Flatpak can't
be built that way. `release.py` adds the Flatpak to the release files as `QuestLHSync-frame-installer.flatpak` when
it's in `out/`.

`lhsyncd`'s core (`src/headset/lhsyncd.c`) is shared by both headsets; each adds
its own side behind `src/headset/headset.h` (`magisk/src/quest.c`,
`frame/src/frame.c`).

## License

MIT, see [LICENSE](LICENSE). Third-party code is listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

QuestLHSync is not affiliated with Meta or Valve. Meta Quest is a trademark of
Meta Platforms, Inc.; Steam Frame and SteamVR are trademarks of Valve
Corporation. Rooting a headset and running code inside its system services is
at your own risk.
