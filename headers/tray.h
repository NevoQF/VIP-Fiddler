#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include <windows.h>
#define WM_TRAYICON (WM_USER + 1)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitTrayIcon(HWND hwnd);
void RemoveTrayIcon();

#endif
