![Icon](https://raw.githubusercontent.com/nefarius/ViGEm/master/Installer/favicon.png)

# ATTENTION! We are moving!
**Hi there. [We're moving to our own organization](https://github.com/ViGEm)!** :smile:

**Don't author any pull-requests on this repository as they will be discarded, thank you.**

# ViGEm
Virtual Gamepad Emulation Framework

This repository contains projects related to USB device emulation and HID device manipulation on the Windows platform.

## Projects
### Kernel-mode
- [XnaGuardian](../../tree/master/Sys/XnaGuardian) - a Windows kernel-mode driver which introduces additional requests to common `XnaComposite` devices.

### User-mode
- [ViGEmTester](../../tree/master/Src/Samples/ViGEmTester) - general purpose test application for debugging purposes.
- [ViGEmTester.NET](../../tree/master/ViGEmTester.NET) - general purpose managed test application for debugging purposes.
- [HidCerberus.Srv](../../tree/master/NET/HidCerberus.Srv) - a Windows Service handling white-listed processes for interaction with `HidGuardian`.
- [HidCerberus.Lib](../../tree/master/Src/HidCerberus.Lib) - a Windows user-mode library for interaction with `HidCerberus.Srv`.
- [XInputExtensions](../../tree/master/Src/XInputExtensions) - a Windows user-mode library for interaction with the `XnaGuardian` driver.

## How to build
Additional build instructions are mentioned in the respective project READMEs, although you'd *at least* need the following tools installed:
 - Visual Studio **2017** ([Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) is just fine)
   ![](https://lh3.googleusercontent.com/-euJV0pWLszY/WfNm8K_p6xI/AAAAAAAAAeE/SiPdU9WS4z0UR76kqRU6Sn38R-K9fa4qgCHMYCw/s0/mstsc_2017-10-27_19-03-42.png)
 - [WDK for Windows 10, version 1709](https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit)

## Sponsors
 - [3dRudder](https://www.3drudder.com/eu/)
 - [Wohlfeil.IT e.U.](https://wohlfeil.it/)
 - [Parsec](https://parsec.tv/)
 - [Rainway, Inc](https://rainway.io/)

## Users of ViGEm
 - [3dRudder](https://www.3drudder.com/eu/)
 - [Parsec](https://parsec.tv/)
 - [GloSC](https://github.com/Alia5/GloSC)
 - [WhiteKnight](https://autohotkey.com/boards/viewtopic.php?t=34890)
 - [InputMapper](https://inputmapper.com/)
 - [Oculus VR, LLC.](https://www.oculus.com/)
 - [Rainway, Inc](https://rainway.io/)
 - [WiimoteHook](https://forum.cemu.info/showthread.php/140-WiimoteHook-Nintendo-Wii-Remote-with-Motion-Rumble-and-Nunchuk-support)
