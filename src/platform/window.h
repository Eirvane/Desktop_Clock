#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME  L"DesktopClockWindowClass"
#define WINDOW_TITLE       L"DesktopClock"

#define WM_TRAYICON        (WM_USER + 100)
#define ID_TRAY_ICON       1001

#define ID_MENU_COLOR_VALUE    2001
#define ID_MENU_COLOR_PANEL    2002
#define ID_MENU_EXIT           2003
#define ID_MENU_TOGGLE_MOVE    2004
#define ID_MENU_TOGGLE_TOPMOST 2005
#define ID_MENU_MODE           2006
#define ID_MENU_MODE_24H       2007
#define ID_MENU_MODE_12H       2008
#define ID_MENU_MODE_SECONDS   2009
#define ID_MENU_MODE_CURRENT    2010
#define ID_MENU_MODE_STOPWATCH  2011
#define ID_MENU_MODE_COUNTDOWN  2012

#define ID_MENU_FONT_BASE   3000
#define ID_MENU_FONT_MAX    3099
#define MAX_FONT_MENU_ITEMS 100

extern HINSTANCE g_hInstance;
extern HWND      g_hClockWnd;

ATOM RegisterClockWindowClass(HINSTANCE hInstance);
HWND CreateClockWindow(HINSTANCE hInstance);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateLayeredWindowContent(HWND hWnd);

#endif /* WINDOW_H */
