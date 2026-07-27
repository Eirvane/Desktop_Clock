#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME  L"DesktopClockWindowClass"
#define WINDOW_TITLE       L"DesktopClock"

/* 托盘图标相关常量 */
#define WM_TRAYICON        (WM_USER + 100)
#define ID_TRAY_ICON       1001

/* 右键菜单命令ID：颜色子菜单两项 + 退出 */
#define ID_MENU_COLOR_VALUE  2001   /* 颜色值：手动输入 HEX */
#define ID_MENU_COLOR_PANEL  2002   /* 颜色面板：系统颜色选择对话框 */
#define ID_MENU_EXIT         2003   /* 退出程序 */

/* 字体菜单动态ID范围（支持最多100个字体文件） */
#define ID_MENU_FONT_BASE   3000
#define ID_MENU_FONT_MAX    3099
#define MAX_FONT_MENU_ITEMS 100

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