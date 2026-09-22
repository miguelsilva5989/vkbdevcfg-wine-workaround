# VKBDevCfg Test-tab crash: Wine exposes 136 buttons for 128-button VKB Gladiator EVO R

## Summary

After granting hidraw access, VKBDevCfg detects and reads the controller. Opening
Test -> Buttons/POVs triggers repeated access-violation dialogs. A standalone
DirectInput probe reports 136 buttons: 128 normal Button-page usages, followed
by eight entries with usage page 0x0001 and usage 0x0000. Filtering these eight
entries through an application-local DirectInput wrapper prevents the observed
Test-tab crash.

## Tested environment

- CachyOS (Arch-based), x86_64, X11.
- System Wine 11.16 (package wine 11.16-2.1).
- Reproduced separately with GE-Proton10-34 (wine-10.0 Staging) in a new prefix.
- VKBDevCfg-C v0.93.89, PE32/i386, official VKB download.
- VKBsim Gladiator EVO R, USB VID:PID 231d:0200.
- Device list reports firmware v2.20C; application title mentions NJoy32 v2.20.D.
- Controller configuration exposes 128 buttons.
- The issue also reproduces with the application's skin disabled.
- Native Windows behavior and newer Wine releases have NOT been tested.

Official app: https://vkbsimcontrollers.com/pages/downloads

## Reproduction

1. Connect the controller and ensure the logged-in desktop user has hidraw access.
   A udev rule used here is:
   SUBSYSTEM=="hidraw", ATTRS{idVendor}=="231d", TAG+="uaccess"
   Place before the system's seat-late rule, reload rules, then reconnect.
2. Run the official configurator under Wine without the custom DirectInput wrapper.
3. Select the controller. Reading configuration succeeds.
4. Open Test -> Buttons/POVs. Repeated access-violation dialogs follow.

The attached read-only probe reproduces the suspicious enumeration without
opening the configurator's crashing Test screen:

    i686-w64-mingw32-gcc -O2 -o vkb-probe.exe tools/vkb-probe.c -ldinput8 -ldxguid
    WINEPREFIX=/path/to/test-prefix WINEDLLOVERRIDES='dinput8=b' wine ./vkb-probe.exe

## Observed enumeration

Before workaround:

    axes=8 buttons=136 POVs=1
    button instance=127 offset=163 usage=0009:0080
    button instance=128 offset=164 usage=0001:0000
    ...
    button instance=135 offset=171 usage=0001:0000

After workaround:

    axes=8 buttons=128 POVs=1
    final button instance=127 offset=163 usage=0009:0080

The attached raw HID report descriptor declares 128 Button-page inputs and also
contains constant fields with undefined usages. Suspected interpretation of
constant/undefined HID fields as extra buttons requires maintainer confirmation.
A native-Windows comparison would help establish expected enumeration precisely.

## Crash evidence

The first access violation is inside VKBDevCfg-C.exe:

    EIP=008ECF1D, EBX=00000000, read address=00000010
    faulting instruction: mov eax, [ebx+0x10]

Disassembly and the captured stack connect the failure to the tester's timer:
button-update loop at 00422B54 -> drawing update at 004E5D8C -> caller
004E5DAA -> fault at 008ECF1D. The drawing update receives a null object pointer.
The button loop follows the reported count. These observations are consistent
with the UI indexing beyond its 128 button indicators; this is an inference
from the binary, not a diagnosis confirmed by VKB source code.

## Local workaround and validation

An application-local dinput8.dll wrapper, limited to VID 231d, caps
GetCapabilities.dwButtons at 128 and filters button instances >=128 from
EnumObjects. Other calls, including axis/POV handling and input-state reads,
are passed through. Source is attached for review. See docs/BUILD.md and build.sh
for a complete source-only build and installation procedure.

It loads a private copy of Wine 11.16's 32-bit dinput8.dll under another name;
the private copy's DOS-stub builtin marker is changed to permit renamed loading.
The launcher selects the local DLLs with:

    WINEDLLOVERRIDES='mscoree,mshtml=;dinput8=n;vkb-wine-dinput8=n'

The official VKB executable, system Wine libraries and controller firmware were
not modified. No controller configuration was written during the investigation.
This is a version-specific workaround, NOT a proposed general 128-button cap
for Wine and NOT an official VKB/Wine fix.

With the wrapper: Buttons/POVs and Axes open, joystick reports arrive, and six
Buttons/Axes switches over 30 seconds pass under a debugger configured to stop
at the first access violation. This is limited validation; configuration writes,
calibration, firmware flashing and every physical button remain untested.

## Requested investigation

Wine: investigate why eight usage 0001:0000 objects are exposed as buttons for
this descriptor. Correct filtering belongs upstream, with tests for this HID
layout and expected Windows behavior.

VKB: bound updates by the allocated UI indicator count and handle unexpected
DirectInput capabilities / null drawing objects without an exception loop.

## Where to post

- Wine bug tracker: https://bugs.winehq.org/enter_bug.cgi?product=Wine
  Suggested component: dinput. Search for duplicates first. Lead with the system
  Wine reproduction; the GE-Proton run is additional evidence. Bug 57030 is
  related VKB context, but its detection/configuration failure is not proven
  to be the same issue as this specific Test-tab crash.
- VKB official English Technical Support:
  https://forum.vkb-sim.pro/viewforum.php?f=25
- After obtaining issue links, share a short workaround summary on r/VKB or
  r/hotas and link back to the technical reports.

The repository contains source and focused evidence. The evidence directory
contains only controller enumeration, the HID descriptor, the official
application hash and a short exception excerpt. It contains no Windows/Wine
binaries, Wine prefix, device serial number or full diagnostic log. A reviewed
application screenshot is displayed separately in the repository README.
