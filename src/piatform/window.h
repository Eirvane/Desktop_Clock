#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME  L"DesktopClockWindowClass"
#define WINDOW_TITLE       L"DesktopClock"

/* 注册窗口类 */
ATOM RegisterClockWindowClass(HINSTANCE hInstance);

/* 创建时钟窗口（无边框、透明、置顶） */
HWND CreateClockWindow(HINSTANCE hInstance);

/* 窗口过程函数 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* WINDOW_H */