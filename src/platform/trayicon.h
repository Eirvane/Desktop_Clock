#ifndef TRAYICON_H
#define TRAYICON_H

#include <windows.h>

/* 初始化系统托盘图标 */
void TrayIcon_Init(HWND hWnd, HINSTANCE hInstance);

/* 移除系统托盘图标（应在程序退出前调用） */
void TrayIcon_Remove(HWND hWnd);

#endif /* TRAYICON_H */