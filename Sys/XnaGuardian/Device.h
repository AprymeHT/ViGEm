/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

#include "public.h"
#include "XInputOverrides.h"
#include "XnaGuardianShared.h"

EXTERN_C_START

#define MAX_HARDWARE_ID_SIZE        0xFF
#define URB_QUEUE_LOCK()            WdfSpinLockAcquire(pDeviceContext->UpperUsbInterruptRequestsLock)
#define URB_QUEUE_UNLOCK()          WdfSpinLockRelease(pDeviceContext->UpperUsbInterruptRequestsLock)

//
// Returns the current caller process id.
// 
#define CURRENT_PROCESS_ID() ((DWORD)((DWORD_PTR)PsGetCurrentProcessId() & 0xFFFFFFFF))

typedef struct _XINPUT_PAD_STATE_INTERNAL
{
    XINPUT_GAMEPAD_OVERRIDES    Overrides;
    XINPUT_GAMEPAD_STATE        Gamepad;

} XINPUT_PAD_STATE_INTERNAL, *PXINPUT_PAD_STATE_INTERNAL;

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    ULONG               MaxDevices;
    UCHAR               LedValues[XINPUT_MAX_DEVICES];
    WDFMEMORY           MemoryHardwareId;
    PCWSTR              HardwareId;
    WDFMEMORY           MemoryClassName;
    PCWSTR              ClassName;
    BOOLEAN             IsXnaDevice;
    BOOLEAN             IsHidUsbDevice;
    WDFQUEUE            UpperUsbInterruptRequests;
    WDFSPINLOCK         UpperUsbInterruptRequestsLock;
    WDFUSBDEVICE        UsbDevice;
    WDFUSBINTERFACE     UsbInterface;
    WDFUSBPIPE          InterruptPipe;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

typedef struct _XINPUT_PAD_IDENTIFIER_CONTEXT
{
    ULONG   Index;

} XINPUT_PAD_IDENTIFIER_CONTEXT, *PXINPUT_PAD_IDENTIFIER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XINPUT_PAD_IDENTIFIER_CONTEXT, GetPadIdentifier)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
XnaGuardianCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EVT_WDF_OBJECT_CONTEXT_CLEANUP XnaGuardianCleanupCallback;

EXTERN_C_END
