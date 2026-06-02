# MiSTer Companion Vita

MiSTer Companion Vita is a small experimental PlayStation Vita homebrew project based on MiSTer Companion NX.

This project is a direct port of MiSTer Companion NX, with several Vita-specific changes to better fit the PlayStation Vita hardware, screen resolution, controls, and homebrew environment. It is not intended to replace or represent the desktop or mobile versions of MiSTer Companion.

## About

This project is focused on interacting with a MiSTer FPGA from a PlayStation Vita.

The goal is simple: bring the lightweight MiSTer Companion NX experience to the Vita while adapting it where needed for the Vita screen, controls, and VitaSDK-based homebrew development. The project uses SSH to connect to a MiSTer and provides a small set of Companion-inspired features adapted for handheld use.

Compared to the Nintendo Switch version, the Vita version uses a scaled layout for the Vita resolution and replaces Switch-specific controls with Vita-friendly alternatives.

## Important Notice

MiSTer Companion Vita is separate from the main MiSTer Companion desktop and mobile apps.

It is also separate from MiSTer Companion NX, even though it is directly based on that project. It may have fewer features, behave differently, or change direction entirely. Unless this project reaches a point where I am personally happy with the results, it should be seen as a personal experiment rather than an actively supported MiSTer Companion release.

Development may be slow, irregular, or stop at any time.

## Current Features

- **Connection tab**: Connect to a MiSTer manually, manage saved profiles, and scan the local network for MiSTer devices.
- **Device tab**: View basic MiSTer device information and perform simple device actions such as reboot handling.
- **Settings tab**: View and adjust supported MiSTer configuration options.
- **Scripts tab**: Run supported MiSTer scripts such as Update All.
- **Remote tab**: Use the PlayStation Vita controls to control MiSTer navigation, including OSD control and supported in-game input.
- **Wallpapers tab**: Browse and manage supported MiSTer wallpaper options.
- **Extras tab**: Install, update, uninstall, and check supported extras such as Zaparoo Frontend and RetroAchievement Cores.

## Vita-Specific Changes

- Ported from devkitPro/libnx to VitaSDK.
- Layout scaled from the Nintendo Switch version to better fit the Vita resolution.
- App title changed to MiSTer Companion Vita.
- Displayed controls adjusted to use PlayStation-style button naming.
- Switch-specific ZL/ZR references replaced with Vita-friendly controls.
- Passthrough mode exit adjusted for the Vita, since the Vita does not have L3/R3 buttons.
- Local configuration paths adjusted for the Vita filesystem.

## Requirements

To build the project, you need a PlayStation Vita homebrew development environment with:

- VitaSDK
- vita2dlib
- libssh2
- OpenSSL
- mbedTLS
- zlib
- libpng
- libjpeg-turbo
- freetype

Some dependencies may need to be installed through `vdpm` or built manually if they are not available in your VitaSDK package setup.

## Building

From the project folder, run:

```sh
make clean
make
```

The build should create:

```text
mister-companion-vita.vpk
```

Copy the `.vpk` file to your Vita and install it with VitaShell.

## Configuration

MiSTer Companion Vita stores its local configuration on the Vita memory card.

Connection profiles can be created from the Connection tab and are saved locally. Profiles include:

- Profile name
- Host / IP address
- Username
- Password

Profiles are intended to make reconnecting to a known MiSTer easier without re-entering connection details every time.

## Disclaimer

MiSTer Companion Vita is an unofficial experimental homebrew project.

It is not affiliated with Sony or PlayStation.