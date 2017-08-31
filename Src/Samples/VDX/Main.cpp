/*
MIT License

Copyright (c) 2017 Benjamin "Nefarius" H�glinger

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

/*
 * TODO:
 * Error checking
 * Vibration feedback
 * Async plugin
 * 
/* */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <Xinput.h>
#include <ViGEmClient.h>
#include <ViGEmUtil.h>
#include "resource.h"


enum EmulationtargetType : int
{
    X360,
    DS4
};

//
// Helper object to keep track of state during every frame
// 
struct EmulationTarget
{
    //
    // Is x360ce reporting a device for the current user index
    // 
    bool isSourceConnected;

    //
    // Is the target device (virtual controller) currently alive
    // 
    bool isTargetConnected;

    //
    // Device object referring to the emulation target
    // 
    PVIGEM_TARGET target;

    //
    // Type of emulated device (from combo-list)
    // 
    // Currently either an X360 pad or a DS4
    // 
    EmulationtargetType targetType;
};

static EmulationTarget targets[XUSER_MAX_COUNT];


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    sf::RenderWindow window(sf::VideoMode(640, 240), "x360ce to ViGEm demo application", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    //
    // Set window icon
    // 
    auto hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon)
    {
        SendMessage(window.getSystemHandle(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
    }

    //
    // Alloc ViGEm client
    // 
    auto client = vigem_alloc();

    //
    // Dynamically load XInput1_3.dll provided by x360ce
    // 
    auto xInputMod = LoadLibrary(L"XInput1_3.dll");

    //
    // Declare XInput functions 
    // 
    auto pXInputEnable = reinterpret_cast<VOID(WINAPI*)(BOOL)>(GetProcAddress(xInputMod, "XInputEnable"));
    auto pXInputGetState = reinterpret_cast<DWORD(WINAPI*)(DWORD, XINPUT_STATE*)>(GetProcAddress(xInputMod, "XInputGetState"));

    vigem_connect(client);
    pXInputEnable(TRUE);

    XINPUT_STATE state;

    window.resetGLStates();
    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        //
        // Create main window
        // 
        ImGui::SetNextWindowSize(ImVec2(550, 130));
        ImGui::SetNextWindowPosCenter();
        ImGui::Begin("", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Columns(4);

        //
        // Header
        // 
        ImGui::Text("Player index");
        ImGui::NextColumn();
        ImGui::Text("Status");
        ImGui::SetColumnOffset(-1, 100);
        ImGui::NextColumn();
        ImGui::Text("Emulation type");
        ImGui::SetColumnOffset(-1, 200);
        ImGui::NextColumn();
        ImGui::Text("Action");
        ImGui::SetColumnOffset(-1, 450);
        ImGui::NextColumn();

        ImGui::Separator();

        //
        // Build main controls
        // 
        for (auto i = 0; i < XUSER_MAX_COUNT; i++)
        {
            ImGui::Text("Player %d", i + 1); ImGui::NextColumn();

            auto ret = pXInputGetState(i, &state);

            targets[i].isSourceConnected = (ret == ERROR_SUCCESS);

            ImGui::Text(targets[i].isSourceConnected ? "Connected" : "Disconnected");

            ImGui::NextColumn();
            ImGui::PushItemWidth(230);
            ImGui::PushID(i);
            ImGui::Combo("##dummy",
                reinterpret_cast<int*>(&targets[i].targetType),
                "Xbox 360 Controller\0DualShock 4 Controller\0\0");
            ImGui::NextColumn();

            if (targets[i].isSourceConnected)
            {
                ImGui::PushID(i);

                auto clicked = ImGui::Button(targets[i].isTargetConnected ? "Disconnect" : "Connect");

                if (clicked)
                {
                    if (targets[i].isTargetConnected)
                    {
                        targets[i].isTargetConnected = !VIGEM_SUCCESS(vigem_target_remove(client, targets[i].target));

                        vigem_target_free(targets[i].target);
                    }
                    else
                    {
                        switch (targets[i].targetType)
                        {
                        case X360:
                            targets[i].target = vigem_target_x360_alloc();
                            break;
                        case DS4:
                            targets[i].target = vigem_target_ds4_alloc();
                            break;
                        }

                        targets[i].isTargetConnected = VIGEM_SUCCESS(vigem_target_add(client, targets[i].target));
                    }
                }
            }

            if (targets[i].isTargetConnected)
            {
                switch (targets[i].targetType)
                {
                case X360:
                    //
                    // The XINPUT_GAMEPAD structure is identical to the XUSB_REPORT structure
                    // so we can simply take it "as-is" and cast it.
                    // 
                    vigem_target_x360_update(client, targets[i].target, *reinterpret_cast<XUSB_REPORT*>(&state.Gamepad));
                    break;
                case DS4:
                {
                    DS4_REPORT rep;
                    DS4_REPORT_INIT(&rep);

                    XUSB_TO_DS4_REPORT(reinterpret_cast<PXUSB_REPORT>(&state.Gamepad), &rep);

                    vigem_target_ds4_update(client, targets[i].target, rep);
                }
                break;
                }
            }

            ImGui::NextColumn();
        }

        ImGui::End();

        window.clear(sf::Color(74, 165, 255));

        ImGui::SFML::Render(window);
        window.display();
    }

    vigem_disconnect(client);
    vigem_free(client);

    ImGui::SFML::Shutdown();
}
