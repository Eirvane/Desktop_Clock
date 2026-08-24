#include "trayicon.h"
#include "window.h"   /* 使用 g_hInstance, g_hClockWnd, WM_TRAYICON, ID_TRAY_ICON */
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

/* 托盘图标数据结构 */
static NOTIFYICONDATAW g_nid = { 0 };

void TrayIcon_Init(HWND hWnd, HINSTANCE hInstance)
{
    /* 同步全局状态（供其他模块使用） */
    g_hInstance = hInstance;
    g_hClockWnd = hWnd;

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, 128, L"桌面时钟");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void TrayIcon_Remove(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}