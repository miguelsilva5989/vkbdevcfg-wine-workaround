# VKBDevCfg Wine Test-tab workaround

A source-only, application-local DirectInput workaround for the VKBDevCfg
Test-tab crash observed with a VKB Gladiator EVO R on Linux.

In the tested setup, Wine reports **136 buttons** for a controller configured
with **128 logical buttons**. The final eight entries have usage `0001:0000`.
Opening VKBDevCfg's Test tab causes repeated access violations. Filtering the
extra entries allows its Buttons/POVs and Axes screens to operate.

This is an **unofficial workaround**, not an upstream Wine patch or an official
VKB release. It does not modify the VKB executable or controller firmware.

## Build and use

Read **[the complete build and installation guide](docs/BUILD.md)**. With an
initialized, dedicated Wine prefix and the 32-bit MinGW-w64 C compiler installed:

```bash
bash build.sh "$HOME/.local/share/wineprefixes/vkb"
```

This creates `build/dinput8.dll` from the included C source and prepares
`build/vkb-wine-dinput8.dll` from **your own Wine installation**. Install both
beside VKBDevCfg and use the DLL overrides documented in the guide.

No Windows executables or DLL binaries are distributed in this repository.
Download VKBDevCfg separately from [VKB's official downloads page](https://vkbsimcontrollers.com/pages/downloads).

## How it works

For VKB devices (USB vendor `231d`) only, the wrapper caps
`IDirectInputDevice8::GetCapabilities().dwButtons` at 128 and excludes button
instances >=128 from `EnumObjects`. Other calls are forwarded to a private
copy of Wine's DirectInput implementation.

The private copy has only its DOS-stub builtin marker changed so it can load
under a separate name. The original Wine library remains unchanged. Rebuild
and retest after changing Wine versions; do not mix Wine and Proton libraries.

This cap is specific to this application's tested limitations. It is not a
proposal to impose a general 128-button limit in Wine.

## Tested scope

- VKBDevCfg-C **v0.93.89** (32-bit).
- System Wine **11.16**, on CachyOS/Arch-based Linux, X11.
- VKB Gladiator EVO R, **231d:0200**, firmware displayed as **v2.20C**.
- The original crash also reproduced with **GE-Proton10-34** and with the
  application's skin disabled. The installed workaround was tested on Wine 11.16.

With the workaround, Buttons/POVs and Axes open, joystick reports arrive, and
six switches between these tabs over 30 seconds pass with a first-exception
crash guard. Every physical button, configuration writes, calibration, firmware
flashing, native Windows behavior and other versions/devices remain unverified.

## Diagnosis and helping upstream

- [Reproduction steps, exception details and suggested upstream reports](docs/BUG-REPORT.md)
- [Read-only DirectInput probe](tools/vkb-probe.c)
- [Before/after enumeration and focused evidence](evidence/)

The evidence suggests Wine exposes undefined/constant HID fields as extra
buttons, while the application does not safely bound its indicator updates.
The exact HID parsing cause and native-Windows behavior need maintainer review.
An upstream enumeration fix and defensive UI handling would remove the need
for this workaround.

For a report, include controller model, VID/PID, firmware, configurator version,
Wine version and focused probe output. Check logs for personal paths or device
identifiers before sharing them. Never upload a Wine prefix or credentials.

## License

The original source and documentation in this repository are MIT-licensed.
Wine and VKB software retain their own licenses and are not redistributed here.
