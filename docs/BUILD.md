# Build the VKBDevCfg Wine workaround yourself

This repository contains source, not DLL binaries. You compile a small
32-bit DirectInput wrapper locally. A second DLL is derived from your own
installed Wine library. Obtain VKBDevCfg separately from VKB's official site.

This workaround was verified with VKBDevCfg v0.93.89, Wine 11.16 and a
Gladiator EVO R (231d:0200), firmware v2.20C. It filters extra button entries
for VKB devices only. It is experimental and is not an official Wine/VKB fix.
Other Wine versions and devices need their own testing. It is not a general
Wine patch imposing a 128-button limit on all applications.

## 1. Requirements

Use a normal Wine installation capable of running 32-bit Windows applications,
Bash, Python 3, and the compiler named `i686-w64-mingw32-gcc`.

On Arch/CachyOS, with Wine already installed:

```bash
sudo pacman -S --needed mingw-w64-gcc python
```

Official package information:
https://archlinux.org/packages/extra/x86_64/mingw-w64-gcc/

On other distributions, install their MinGW-w64 i686 C compiler and Python 3.
The package names vary. The DLL must be compiled for **i686/32-bit**, even
on a 64-bit Linux machine, because VKBDevCfg itself is a 32-bit application.

## 2. Prepare a dedicated Wine prefix and the official application

These example paths are in your home directory; choose your own if preferred.
Use the same Wine installation to initialize the prefix and run the app.
Do not point these commands at a game/Steam prefix.

```bash
export WINEPREFIX="$HOME/.local/share/wineprefixes/vkb"
mkdir -p "$WINEPREFIX" "$HOME/Applications/VKB"
WINEDLLOVERRIDES='mscoree,mshtml=' wineboot -u
```

Download the configurator ZIP from:
https://vkbsimcontrollers.com/pages/downloads

Extract it into `$HOME/Applications/VKB` so that the executable is located at
`$HOME/Applications/VKB/VKBDevCfg-C.exe`.

## 3. Compile and prepare the two DLLs

Open a terminal in the repository root, then run:

```bash
bash build.sh "$HOME/.local/share/wineprefixes/vkb"
```

The script checks for the correct i386 PE Wine library, compiles the wrapper,
and creates:

- `build/dinput8.dll`: compiled from the attached `src/dinput8-vkb.c` source.
- `build/vkb-wine-dinput8.dll`: a private copy of YOUR Wine DirectInput DLL.
- `build/build-info.json`: source path and hashes for the private copy.

The second file is NOT compiled by this script. It copies Wine's existing
32-bit implementation and changes its 16-byte DOS-stub builtin marker so Wine
can load it under a new name. No executable instructions in that copied Wine
library are patched. The Wine prefix's original library is unchanged.

The underlying compiler command is:

```bash
i686-w64-mingw32-gcc -shared -O2 -Wl,--kill-at \
  -o build/dinput8.dll src/dinput8-vkb.c src/dinput8.def
```

`build.sh` additionally prepares the second DLL and performs validation.

## 4. Install beside VKBDevCfg and launch

Close VKBDevCfg before copying the files. Both DLLs go into its application
folder. If that folder already contains a different dinput8.dll, use a fresh
application folder rather than overwriting another workaround.

```bash
cp build/dinput8.dll build/vkb-wine-dinput8.dll "$HOME/Applications/VKB/"
cd "$HOME/Applications/VKB"
WINEPREFIX="$HOME/.local/share/wineprefixes/vkb" \
WINEDLLOVERRIDES='mscoree,mshtml=;dinput8=n;vkb-wine-dinput8=n' \
wine ./VKBDevCfg-C.exe
```

The override tells Wine to load the wrapper from the application directory.
The wrapper forwards to the private Wine implementation after filtering VKB
button capability/enumeration results. This does not flash firmware or write
controller settings.

## 5. USB access is a separate requirement

If VKBDevCfg cannot detect the controller, inspect the device's hidraw access.
On a system using systemd-logind, the rule used in our test was:

```text
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="231d", TAG+="uaccess"
```

Store it as `/etc/udev/rules.d/70-vkb-hidraw.rules`, reload the rules with
`sudo udevadm control --reload-rules`, then unplug and reconnect the joystick.
The rule must run before the system's seat-late/uaccess processing.

## 6. Verify and stop

Select the controller and open Test -> Buttons/POVs and Test -> Axes. Check
that the screen remains stable and your controls respond. Our validation
covered opening these tabs and six switches over 30 seconds; every physical
button, writing settings, calibration and firmware flashing remain unverified.

If dialogs recur, stop only this dedicated Wine prefix:

```bash
WINEPREFIX="$HOME/.local/share/wineprefixes/vkb" wineserver -k
```

For an independent enumeration check, compile `tools/vkb-probe.c` and run it
beside the two DLLs, using the same override. The probe should show 128 buttons
for the tested controller. See the main bug report for the original result.

## 7. Undo or update

To undo, close the app and move ONLY the two workaround DLLs out of its folder.
Remove the `dinput8=n;vkb-wine-dinput8=n` overrides from your launch command.
The USB access rule is independent and can remain installed.

After changing Wine versions, close VKBDevCfg, update the dedicated prefix with
that version's `wineboot -u`, rerun `build.sh`, and reinstall both local DLLs.
Do not reuse the private library across Wine/Proton versions without rebuilding
and retesting. A future Wine fix may make the wrapper unnecessary.

## Sharing

Share the repository link, including its source, report and build script.
Recipients generate both DLLs using their own environment. Do not describe
this as an upstream fix or universal compatibility guarantee. Generated DLLs, EXEs and Wine prefixes are excluded from version control.
