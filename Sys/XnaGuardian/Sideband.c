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


#include "Sideband.h"
#include "sideband.tmh"
#include <wdmsec.h>

WDFCOLLECTION   FilterDeviceCollection;
WDFWAITLOCK     FilterDeviceCollectionLock;
WDFDEVICE       ControlDevice = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, FilterCreateControlDevice)
#pragma alloc_text (PAGE, FilterDeleteControlDevice)
#pragma alloc_text (PAGE, XnaGuardianSidebandFileCleanup)
#endif

//
// Creates the control device for sideband communication.
// 
_Use_decl_annotations_
NTSTATUS
FilterCreateControlDevice(
    WDFDEVICE Device
)
{
    PWDFDEVICE_INIT             pInit;
    WDFDEVICE                   controlDevice = NULL;
    WDF_OBJECT_ATTRIBUTES       controlAttributes;
    WDF_IO_QUEUE_CONFIG         ioQueueConfig;
    BOOLEAN                     bCreate = FALSE;
    NTSTATUS                    status;
    WDFQUEUE                    queue;
    WDF_FILEOBJECT_CONFIG       foCfg;
    DECLARE_CONST_UNICODE_STRING(ntDeviceName, NTDEVICE_NAME_STRING);
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, SYMBOLIC_NAME_STRING);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_SIDEBAND, "%!FUNC! Entry");

    PAGED_CODE();

    //
    // First find out whether any ControlDevice has been created. If the
    // collection has more than one device then we know somebody has already
    // created or in the process of creating the device.
    //
    WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

    if (WdfCollectionGetCount(FilterDeviceCollection) == 1) {
        bCreate = TRUE;
    }

    WdfWaitLockRelease(FilterDeviceCollectionLock);

    if (!bCreate) {
        //
        // Control device is already created. So return success.
        //
        return STATUS_SUCCESS;
    }

    KdPrint((DRIVERNAME "Creating Control Device\n"));

    //
    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    pInit = WdfControlDeviceInitAllocate(
        WdfDeviceGetDriver(Device),
        &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
    );

    if (pInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Set exclusive to false so that more than one app can talk to the
    // control device simultaneously.
    //
    WdfDeviceInitSetExclusive(pInit, FALSE);

    status = WdfDeviceInitAssignName(pInit, &ntDeviceName);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    WDF_FILEOBJECT_CONFIG_INIT(&foCfg, NULL, NULL, XnaGuardianSidebandFileCleanup);
    WdfDeviceInitSetFileObjectConfig(pInit, &foCfg, WDF_NO_OBJECT_ATTRIBUTES);

    //
    // Specify the size of device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&controlAttributes);
    status = WdfDeviceCreate(&pInit,
        &controlAttributes,
        &controlDevice);
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Create a symbolic link for the control object so that user-mode can open
    // the device.
    //

    status = WdfDeviceCreateSymbolicLink(controlDevice,
        &symbolicLinkName);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Configure the default queue associated with the control device object
    // to be Serial so that request passed to EvtIoDeviceControl are serialized.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
        WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoDeviceControl = XnaGuardianSidebandIoDeviceControl;

    //
    // Framework by default creates non-power managed queues for
    // filter drivers.
    //
    status = WdfIoQueueCreate(controlDevice,
        &ioQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &queue // pointer to default queue
    );
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Control devices must notify WDF when they are done initializing.   I/O is
    // rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

    ControlDevice = controlDevice;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_SIDEBAND, "%!FUNC! Exit");

    return STATUS_SUCCESS;

Error:

    if (pInit != NULL) {
        WdfDeviceInitFree(pInit);
    }

    if (controlDevice != NULL) {
        //
        // Release the reference on the newly created object, since
        // we couldn't initialize it.
        //
        WdfObjectDelete(controlDevice);
        controlDevice = NULL;
    }

    TraceEvents(TRACE_LEVEL_ERROR, TRACE_SIDEBAND, "%!FUNC! Exit - failed with status %!STATUS!", status);

    return status;
}

//
// Deletes the control device.
// 
_Use_decl_annotations_
VOID
FilterDeleteControlDevice(
    WDFDEVICE Device
)
{
    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    KdPrint((DRIVERNAME "Deleting Control Device\n"));

    if (ControlDevice) {
        WdfObjectDelete(ControlDevice);
        ControlDevice = NULL;
    }
}

//
// Handles requests sent to the sideband control device.
// 
#pragma warning(push)
#pragma warning(disable:28118) // this callback will run at IRQL=PASSIVE_LEVEL
_Use_decl_annotations_
VOID XnaGuardianSidebandIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    NTSTATUS                        status = STATUS_INVALID_PARAMETER;
    size_t                          buflen;
    PVOID                           pBuffer;
    PXINPUT_EXT_OVERRIDE_GAMEPAD    pOverride;
    PXINPUT_EXT_PEEK_GAMEPAD        pPeek;
    UCHAR                           userIndex;
    PXINPUT_PAD_STATE_INTERNAL      pPad;
    WDFREQUEST                      UsbRequest;
    PUCHAR                          pUpperBuffer;
    ULONG                           upperBufferLength;
    BOOLEAN                         ret;
    PX360_HID_USB_INPUT_REPORT      pX360Report;
    PXBONE_HID_USB_INPUT_REPORT     pXboneReport;

    KdPrint((DRIVERNAME "XnaGuardianSidebandIoDeviceControl called with code 0x%X\n", IoControlCode));

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(pX360Report);

    switch (IoControlCode)
    {
#pragma region IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE
    case IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE\n"));

        // 
        // Retrieve input buffer
        // 
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD), &pBuffer, &buflen);
        if (!NT_SUCCESS(status) || buflen < sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed with status 0x%X\n", status));
            break;
        }

        pOverride = (PXINPUT_EXT_OVERRIDE_GAMEPAD)pBuffer;

        //
        // Validate padding
        // 
        if (pOverride->Size != sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // 
        // Validate range
        // 
        if (!VALID_USER_INDEX(pOverride->UserIndex))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Set pad overrides
        // 
        pPad = &PadStates[pOverride->UserIndex];
        pPad->Overrides = pOverride->Overrides;
        pPad->Gamepad = pOverride->Gamepad;

        ret = GetUpperUsbRequest(
            WdfCollectionGetItem(HidUsbDeviceCollection, pOverride->UserIndex),
            &UsbRequest,
            &pUpperBuffer,
            &upperBufferLength);

        if (ret)
        {
            KdPrint((DRIVERNAME "GetUpperUsbRequest succeeded\n"));

            if (upperBufferLength == XBONE_HID_USB_INPUT_REPORT_BUFFER_LENGTH)
            {
                KdPrint((DRIVERNAME "Report is XBONE Report\n"));
                KdPrint((DRIVERNAME "BUTTON_OVERRIDES: 0x%X\n", pPad->Overrides));

                pXboneReport = (PXBONE_HID_USB_INPUT_REPORT)pUpperBuffer;

#ifdef DBG
                KdPrint((DRIVERNAME "SIDEBAND_BUFFER_BEFORE: "));
                for (ULONG i = 0; i < upperBufferLength; i++)
                {
                    KdPrint(("%02X ", pUpperBuffer[i]));
                }
                KdPrint(("\n"));
#endif

                XINPUT_GAMEPAD_TO_XBONE_HID_USB_INPUT_REPORT(pPad, pXboneReport);

#ifdef DBG
                KdPrint((DRIVERNAME "SIDEBAND_BUFFER_AFTER: "));
                for (ULONG i = 0; i < upperBufferLength; i++)
                {
                    KdPrint(("%02X ", pUpperBuffer[i]));
                }
                KdPrint(("\n"));
#endif
            }

            WdfRequestComplete(UsbRequest, STATUS_SUCCESS);
        }

        status = STATUS_SUCCESS;

        break;
#pragma endregion

#pragma region IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE
    case IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE\n"));

        // 
        // Retrieve input buffer
        // 
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XINPUT_EXT_PEEK_GAMEPAD), &pBuffer, &buflen);
        if (!NT_SUCCESS(status) || buflen < sizeof(XINPUT_EXT_PEEK_GAMEPAD))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed with status 0x%X\n", status));
            break;
        }

        pPeek = (PXINPUT_EXT_PEEK_GAMEPAD)pBuffer;

        //
        // Validate padding
        // 
        if (pPeek->Size != sizeof(XINPUT_EXT_PEEK_GAMEPAD))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        userIndex = pPeek->UserIndex;

        // 
        // Validate range
        // 
        if (!VALID_USER_INDEX(userIndex))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // 
        // Retrieve output buffer
        // 
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(XINPUT_GAMEPAD_STATE), &pBuffer, &buflen);
        if (!NT_SUCCESS(status) || buflen < sizeof(XINPUT_GAMEPAD_STATE))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveOutputBuffer failed with status 0x%X\n", status));
            break;
        }

        RtlCopyBytes(pBuffer, &PeekPadCache[userIndex], sizeof(XINPUT_GAMEPAD_STATE));

        WdfRequestCompleteWithInformation(Request, status, sizeof(XINPUT_GAMEPAD_STATE));
        return;
#pragma endregion 

    default:
        break;
    }

    WdfRequestComplete(Request, status);
}
#pragma warning(pop) // enable 28118 again

_Use_decl_annotations_
VOID
XnaGuardianSidebandFileCleanup(
    WDFFILEOBJECT  FileObject
)
{
    ULONG   index;

    UNREFERENCED_PARAMETER(FileObject);

    KdPrint((DRIVERNAME "XnaGuardianSidebandFileCleanup called\n"));

    for (index = 0; index < XINPUT_MAX_DEVICES; index++)
    {
        RtlZeroBytes(&PadStates[index], sizeof(XINPUT_PAD_STATE_INTERNAL));
    }
}

