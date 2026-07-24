#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME  L"DesktopClockWindowClass"
#define WINDOW_TITLE       L"DesktopClock"

/* 托盘图标相关常量 */
#define WM_TRAYICON        (WM_USER + 100)
#define ID_TRAY_ICON       1001
#define ID_MENU_SETTINGS   2001
#define ID_MENU_EXIT       2002

/* 注册窗口类 */
ATOM RegisterClockWindowClass(HINSTANCE hInstance);

/* 创建时钟窗口（无边框、透明、置顶） */
HWND CreateClockWindow(HINSTANCE hInstance);

/* 窗口过程函数 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* 系统托盘图标操作 */
void TrayIcon_Init(HWND hWnd, HINSTANCE hInstance);
void TrayIcon_Remove(HWND hWnd);

#endif /* WINDOW_H */