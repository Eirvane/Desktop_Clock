#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME  L"DesktopClockWindowClass"
#define WINDOW_TITLE       L"DesktopClock"

/* 托盘回调消息与图标ID */
#define WM_TRAYICON        (WM_USER + 100)
#define ID_TRAY_ICON       1001

/* 右键菜单命令ID */
#define ID_MENU_COLOR_VALUE  2001   /* HEX 手动输入 */
#define ID_MENU_COLOR_PANEL  2002   /* 系统颜色面板 */
#define ID_MENU_EXIT         2003   /* 退出程序 */

/* 字体菜单动态ID范围（供 WindowProc 做范围判断） */
#define ID_MENU_FONT_BASE   3000
#define ID_MENU_FONT_MAX    3099
#define MAX_FONT_MENU_ITEMS 100

/* ============================================================
 * 全局状态（定义在 window.c，其他模块通过 extern 使用）
 * ============================================================ */
extern HINSTANCE g_hInstance;
extern HWND      g_hClockWnd;

/* 注册窗口类 */
ATOM RegisterClockWindowClass(HINSTANCE hInstance);

/* 创建主窗口（无边框、分层透明、可选置顶） */
HWND CreateClockWindow(HINSTANCE hInstance);

/* 主窗口过程 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* 刷新分层窗口内容（供外部模块调用，如修改颜色/字体后重绘） */
void UpdateLayeredWindowContent(HWND hWnd);

#endif /* WINDOW_H */