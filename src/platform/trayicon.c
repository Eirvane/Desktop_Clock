#include "trayicon.h"
#include "window.h"   /* 使用 g_hInstance, g_hClockWnd, WM_TRAYICON, ID_TRAY_ICON */
#include "../../resource.h"
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

static NOTIFYICONDATAW g_nid = { 0 };

void TrayIcon_Init(HWND hWnd, HINSTANCE hInstance)
{
    g_hInstance = hInstance;
    g_hClockWnd = hWnd;

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTSIZE);
    wcscpy_s(g_nid.szTip, 128, L"桌面时钟");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void TrayIcon_Remove(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}