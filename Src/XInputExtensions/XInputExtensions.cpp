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


#include "stdafx.h"
#include "XInputExtensions.h"
#include <winioctl.h>
#include "XnaGuardianShared.h"

HANDLE                          g_hGuardian = INVALID_HANDLE_VALUE;
XINPUT_EXT_OVERRIDE_GAMEPAD     PadOverrides[XINPUT_MAX_DEVICES];

DWORD OpenGuardian()
{
    if (g_hGuardian != INVALID_HANDLE_VALUE)
    {
        return ERROR_SUCCESS;
    }

    g_hGuardian = CreateFile(XNA_GUARDIAN_DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        0, // FILE_SHARE_READ | FILE_SHARE_WRITE
        nullptr, // no SECURITY_ATTRIBUTES structure
        OPEN_EXISTING, // No special create flags
        0, // No special attributes
        nullptr); // No template file

    if (g_hGuardian != INVALID_HANDLE_VALUE)
    {
        for (auto i = 0; i < XINPUT_MAX_DEVICES; i++)
        {
            XINPUT_EXT_OVERRIDE_GAMEPAD_INIT(&PadOverrides[i], i);
        }

        return ERROR_SUCCESS;
    }

    return GetLastError();
}

XINPUTEXTENSIONS_API DWORD XInputOverrideSetMask(DWORD dwUserIndex, DWORD dwMask)
{
    PXINPUT_EXT_OVERRIDE_GAMEPAD    pPad = nullptr;
    DWORD                           retval = 0;

    if (!VALID_USER_INDEX(dwUserIndex)) return ERROR_BAD_ARGUMENTS;

    if (!SUCCEEDED(HRESULT_FROM_WIN32(OpenGuardian()))) return GetLastError();

    pPad = &PadOverrides[dwUserIndex];

    pPad->Overrides = dwMask;

    auto ret = DeviceIoControl(
        g_hGuardian,
        IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE,
        static_cast<LPVOID>(pPad),
        pPad->Size,
        nullptr,
        0,
        &retval,
        nullptr);

    if (ret > 0) return ERROR_SUCCESS;

    CloseHandle(g_hGuardian);
    g_hGuardian = nullptr;

    return GetLastError();
}

XINPUTEXTENSIONS_API DWORD XInputOverrideSetState(DWORD dwUserIndex, PXINPUT_GAMEPAD pGamepad)
{
    PXINPUT_EXT_OVERRIDE_GAMEPAD    pPad = nullptr;
    DWORD                           retval = 0;

    if (!pGamepad) return ERROR_BAD_ARGUMENTS;

    if (!VALID_USER_INDEX(dwUserIndex)) return ERROR_BAD_ARGUMENTS;

    if (!SUCCEEDED(HRESULT_FROM_WIN32(OpenGuardian()))) return GetLastError();

    pPad = &PadOverrides[dwUserIndex];

    pPad->Gamepad = *reinterpret_cast<PXINPUT_GAMEPAD_STATE>(pGamepad);

    auto ret = DeviceIoControl(
        g_hGuardian,
        IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE,
        static_cast<LPVOID>(pPad),
        pPad->Size,
        nullptr,
        0,
        &retval,
        nullptr);

    if (ret > 0) return ERROR_SUCCESS;

    CloseHandle(g_hGuardian);
    g_hGuardian = nullptr;

    return GetLastError();
}

XINPUTEXTENSIONS_API DWORD XInputOverridePeekState(DWORD dwUserIndex, PXINPUT_GAMEPAD pGamepad)
{
    XINPUT_EXT_PEEK_GAMEPAD     peek;
    DWORD                       retval = 0;

    if (!pGamepad) return ERROR_BAD_ARGUMENTS;

    if (!VALID_USER_INDEX(dwUserIndex)) return ERROR_BAD_ARGUMENTS;

    if (!SUCCEEDED(HRESULT_FROM_WIN32(OpenGuardian()))) return GetLastError();

    XINPUT_EXT_PEEK_GAMEPAD_INIT(&peek, dwUserIndex);

    auto ret = DeviceIoControl(
        g_hGuardian,
        IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE,
        static_cast<LPVOID>(&peek),
        peek.Size,
        pGamepad,
        sizeof(XINPUT_GAMEPAD),
        &retval,
        nullptr);

    if (ret > 0) return ERROR_SUCCESS;

    CloseHandle(g_hGuardian);
    g_hGuardian = nullptr;

    return GetLastError();
}


