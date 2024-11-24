#include "../headers/config.h"
#include "../headers/globals.h"
#include "../headers/funcs.h"
#include "../headers/tray.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <chrono>

std::string ROBLOSECURITY;
std::string KEYBIND;
std::string csrfTokenCache;
long long csrfTokenLastFetched = 0;

int main() {
    const std::string configFilename = "config.json";

    if (!fileExists(configFilename)) {
        createDefaultConfig(configFilename);
        return 1;
    }

    KEYBIND = readKeybind(configFilename);
    ROBLOSECURITY = readRobloxCookie(configFilename);
    bool runInTray = readRunInTray(configFilename);

    if (ROBLOSECURITY.empty()) {
        std::cout << "Please update " << configFilename << " with your ROBLOX_COOKIE.\n";
        return 1;
    }

    auto userInfo = getUserIDAndUsernameFromCookie(ROBLOSECURITY);
    std::string userID = userInfo.first;
    std::string username = userInfo.second;

    if (userID.empty() || username.empty()) {
        std::cout << "Failed to retrieve user info. Please check your ROBLOSECURITY cookie.\n";
        return 1;
    }
    std::cout << "User ID: " << userID << ", Username: " << username << "\n";

    std::thread keybindThread(handleKeybindPress, KEYBIND);

    if (runInTray) {
        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = TEXT("TrayWindowClass");

        if (!RegisterClass(&wc)) {
            std::cout << "Failed to register window class.\n";
            return 1;
        }

        HWND hwnd = CreateWindowEx(
            0,
            TEXT("TrayWindowClass"),
            TEXT("Tray Window"),
            0,
            0, 0, 0, 0,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        if (!hwnd) {
            std::cout << "Failed to create window.\n";
            return 1;
        }

        InitTrayIcon(hwnd);

        ShowWindow(GetConsoleWindow(), SW_HIDE);

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        RemoveTrayIcon();

        keybindThread.detach();
        return 0;
    }
    else {
        keybindThread.join();
    }

    return 0;
}
