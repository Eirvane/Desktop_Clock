#include "window.h"
#include "../graphics/renderer.h"
#include "../utils/config.h"
#include "../utils/utils.h"
#include "trayicon.h"
#include "../ui/fontmenu.h"
#include "../ui/colorpicker.h"
#include "../ui/countdown.h"
#include <windowsx.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_DEFAULT
#define DWMWCP_DEFAULT    0
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND      2
#endif

#ifndef MN_GETHMENU
#define MN_GETHMENU 0x01E1
#endif

HINSTANCE g_hInstance = NULL;
HWND      g_hClockWnd = NULL;

static POINT g_dragStartPt = { 0 };
static RECT  g_dragStartRc = { 0 };

/* 托盘菜单句柄（在菜单显示期间有效，用于 WM_COMMAND 中实时更新） */
static HMENU g_hTrayMenu = NULL;
static HMENU g_hDispMenu = NULL;
static HMENU g_hModeMenu = NULL;
static BOOL  g_bTrayExitPending = FALSE;

/* 菜单消息钩子：点击菜单项（含子菜单）时不关闭菜单 */
static HHOOK g_hMenuHook = NULL;

static LRESULT CALLBACK MenuFilterHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == MSGF_MENU) {
        MSG* pMsg = (MSG*)lParam;
        if (pMsg->message == WM_LBUTTONUP) {
            HWND hMenuWnd = pMsg->hwnd;
            HMENU hMenu = (HMENU)SendMessageW(hMenuWnd, MN_GETHMENU, 0, 0);
            if (hMenu) {
                POINT pt = pMsg->pt;
                ScreenToClient(hMenuWnd, &pt);
                int idx = MenuItemFromPoint(hMenuWnd, hMenu, pt);
                if (idx >= 0) {
                    UINT id = GetMenuItemID(hMenu, idx);
                    if (id != (UINT)-1 && id != 0) {
                        PostMessageW(g_hClockWnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
                        return 1;
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_hMenuHook, nCode, wParam, lParam);
}

ATOM RegisterClockWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
    return RegisterClassExW(&wcex);
}

HWND CreateClockWindow(HINSTANCE hInstance)
{
    DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    if (g_config.topMost) {
        dwExStyle |= WS_EX_TOPMOST;
    }
    if (!g_config.movable) {
        dwExStyle |= WS_EX_TRANSPARENT;
    }

    HWND hWnd = CreateWindowExW(
        dwExStyle,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_POPUP,
        g_config.x,
        g_config.y,
        g_config.width,
        g_config.height,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd) {
        g_hClockWnd = hWnd;
        int cornerPref = g_config.movable ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

        if (!g_config.topMost) {
            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    return hWnd;
}

void UpdateLayeredWindowContent(HWND hWnd)
{
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hbmMem = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hbmMem) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
    Renderer_DrawClock(hdcMem, width, height);

    POINT ptSrc = { 0, 0 };
    POINT ptDst = { rcWindow.left, rcWindow.top };
    SIZE  size = { width, height };
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        UpdateLayeredWindowContent(hWnd);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID_UPDATE) {
            UpdateLayeredWindowContent(hWnd);
        }
        return 0;

    case WM_COMMAND:
    {
        UINT id = LOWORD(wParam);

        if (lParam == 0) {
            switch (id) {
            case ID_MENU_EXIT:
                g_bTrayExitPending = TRUE;
                break;

            case ID_MENU_COLOR_VALUE:
                ColorValueInput_Show(g_hClockWnd);
                break;

            case ID_MENU_COLOR_PANEL:
                ColorPicker_Show(g_hClockWnd);
                break;

            case ID_MENU_MODE_CURRENT:
                g_config.mode = 0;
                Renderer_SetMode(0);
                Config_Save();
                UpdateLayeredWindowContent(g_hClockWnd);
                if (g_hModeMenu) {
                    CheckMenuItem(g_hModeMenu, ID_MENU_MODE_CURRENT, MF_BYCOMMAND | MF_CHECKED);
                    CheckMenuItem(g_hModeMenu, ID_MENU_MODE_STOPWATCH, MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(g_hModeMenu, ID_MENU_MODE_COUNTDOWN, MF_BYCOMMAND | MF_UNCHECKED);
                }
                break;

            case ID_MENU_MODE_STOPWATCH:
                StopwatchStart_Show(g_hClockWnd);
                break;

            case ID_MENU_MODE_COUNTDOWN:
                CountdownSettings_Show(g_hClockWnd);
                break;

            case ID_MENU_MODE_24H:
                g_config.hourFormat = 0;
                Config_Save();
                UpdateLayeredWindowContent(g_hClockWnd);
                if (g_hDispMenu) {
                    CheckMenuItem(g_hDispMenu, ID_MENU_MODE_24H, MF_BYCOMMAND | MF_CHECKED);
                    CheckMenuItem(g_hDispMenu, ID_MENU_MODE_12H, MF_BYCOMMAND | MF_UNCHECKED);
                }
                break;

            case ID_MENU_MODE_12H:
                g_config.hourFormat = 1;
                Config_Save();
                UpdateLayeredWindowContent(g_hClockWnd);
                if (g_hDispMenu) {
                    CheckMenuItem(g_hDispMenu, ID_MENU_MODE_24H, MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(g_hDispMenu, ID_MENU_MODE_12H, MF_BYCOMMAND | MF_CHECKED);
                }
                break;

            case ID_MENU_MODE_SECONDS:
                g_config.showSeconds = !g_config.showSeconds;
                Config_Save();
                UpdateLayeredWindowContent(g_hClockWnd);
                if (g_hDispMenu) {
                    CheckMenuItem(g_hDispMenu, ID_MENU_MODE_SECONDS,
                        MF_BYCOMMAND | (g_config.showSeconds ? MF_CHECKED : MF_UNCHECKED));
                    ModifyMenuW(g_hDispMenu, ID_MENU_MODE_SECONDS, MF_BYCOMMAND | MF_STRING,
                        ID_MENU_MODE_SECONDS, g_config.showSeconds ? L"显示秒" : L"不显示秒");
                }
                break;

            case ID_MENU_TOGGLE_TOPMOST:
                g_config.topMost = !g_config.topMost;
                Config_Save();
                SetWindowPos(hWnd,
                    g_config.topMost ? HWND_TOPMOST : HWND_BOTTOM,
                    0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                /* 【改动】只更新勾选标记，不再切换文字 */
                if (g_hTrayMenu) {
                    CheckMenuItem(g_hTrayMenu, ID_MENU_TOGGLE_TOPMOST,
                        MF_BYCOMMAND | (g_config.topMost ? MF_CHECKED : MF_UNCHECKED));
                }
                break;

            case ID_MENU_TOGGLE_MOVE:
                g_config.movable = !g_config.movable;
                Config_Save();

                {
                    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
                    if (g_config.movable) {
                        exStyle &= ~WS_EX_TRANSPARENT;
                    }
                    else {
                        exStyle |= WS_EX_TRANSPARENT;
                    }
                    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);

                    int cornerPref = g_config.movable ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
                    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

                    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
                }

                if (g_hTrayMenu) {
                    ModifyMenuW(g_hTrayMenu, ID_MENU_TOGGLE_MOVE, MF_BYCOMMAND | MF_STRING,
                        ID_MENU_TOGGLE_MOVE, g_config.movable ? L"正在移动" : L"已固定");
                }
                UpdateLayeredWindowContent(g_hClockWnd);
                break;

            case ID_MENU_OPEN_FONT_FOLDER:
                OpenFontsFolder();
                break;

            default:
                if (id >= ID_MENU_FONT_BASE && id <= ID_MENU_FONT_MAX) {
                    HandleFontMenuCommand(id);
                }
                break;
            }
        }
        return 0;
    }

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            g_hTrayMenu = hMenu;

            /* 1. 字体 */
            HMENU hFontMenu = BuildFontSubmenu();
            if (hFontMenu) {
                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFontMenu, L"字体");
            }

            /* 2. 颜色 */
            HMENU hColorMenu = CreatePopupMenu();
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_VALUE, L"颜色值");
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_PANEL, L"颜色面板");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hColorMenu, L"颜色");

            /* 3. 显示 */
            HMENU hDispMenu = CreatePopupMenu();
            g_hDispMenu = hDispMenu;
            AppendMenuW(hDispMenu, MF_STRING | (g_config.hourFormat == 0 ? MF_CHECKED : 0),
                ID_MENU_MODE_24H, L"24小时制");
            AppendMenuW(hDispMenu, MF_STRING | (g_config.hourFormat == 1 ? MF_CHECKED : 0),
                ID_MENU_MODE_12H, L"12小时制");
            AppendMenuW(hDispMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hDispMenu, MF_STRING | (g_config.showSeconds ? MF_CHECKED : 0),
                ID_MENU_MODE_SECONDS, g_config.showSeconds ? L"显示秒" : L"不显示秒");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hDispMenu, L"显示");

            /* 分隔线：显示 与 模式 之间 */
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            /* 4. 模式 */
            HMENU hModeMenu = CreatePopupMenu();
            g_hModeMenu = hModeMenu;
            AppendMenuW(hModeMenu, MF_STRING | (g_config.mode == 0 ? MF_CHECKED : 0),
                ID_MENU_MODE_CURRENT, L"当前时间");
            AppendMenuW(hModeMenu, MF_STRING | (g_config.mode == 1 ? MF_CHECKED : 0),
                ID_MENU_MODE_STOPWATCH, L"正计时");
            AppendMenuW(hModeMenu, MF_STRING | (g_config.mode == 2 ? MF_CHECKED : 0),
                ID_MENU_MODE_COUNTDOWN, L"倒计时");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hModeMenu, L"模式");

            /* 5. 置顶 */
            /* 【改动】文字固定为"置顶"，通过 MF_CHECKED 表示状态 */
            AppendMenuW(hMenu, MF_STRING | (g_config.topMost ? MF_CHECKED : 0),
                ID_MENU_TOGGLE_TOPMOST, L"置顶");

            /* 6. 固定 */
            AppendMenuW(hMenu, MF_STRING, ID_MENU_TOGGLE_MOVE,
                g_config.movable ? L"正在移动" : L"已固定");

            /* 分隔线：固定 与 退出 之间 */
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            /* 7. 退出 */
            AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"退出");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);

            g_bTrayExitPending = FALSE;
            g_hMenuHook = SetWindowsHookEx(WH_MSGFILTER, MenuFilterHook,
                g_hInstance, GetCurrentThreadId());

            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN,
                pt.x, pt.y, 0, hWnd, NULL);

            UnhookWindowsHookEx(g_hMenuHook);
            g_hMenuHook = NULL;

            DestroyMenu(hMenu);
            g_hTrayMenu = NULL;
            g_hDispMenu = NULL;
            g_hModeMenu = NULL;

            if (g_bTrayExitPending) {
                g_bTrayExitPending = FALSE;
                DestroyWindow(hWnd);
            }
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_config.movable) {
            SetCapture(hWnd);
            GetCursorPos(&g_dragStartPt);
            GetWindowRect(hWnd, &g_dragStartRc);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_config.movable && GetCapture() == hWnd) {
            POINT pt;
            GetCursorPos(&pt);
            int dx = pt.x - g_dragStartPt.x;
            int dy = pt.y - g_dragStartPt.y;
            SetWindowPos(
                hWnd, NULL,
                g_dragStartRc.left + dx,
                g_dragStartRc.top + dy,
                0, 0,
                SWP_NOSIZE | SWP_NOZORDER
            );
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_config.movable && GetCapture() == hWnd) {
            ReleaseCapture();
            RECT rc;
            GetWindowRect(hWnd, &rc);
            g_config.x = rc.left;
            g_config.y = rc.top;
        }
        return 0;

    case WM_MOUSEWHEEL:
    {
        if (!g_config.movable) {
            return 0;
        }

        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int step = (zDelta > 0) ? 2 : -2;
        int newSize = g_config.fontSize + step;

        if (newSize < 8)  newSize = 8;
        if (newSize > 200) newSize = 200;

        if (newSize != g_config.fontSize) {
            g_config.fontSize = newSize;

            RECT rc;
            GetWindowRect(hWnd, &rc);
            int cx = rc.left + (rc.right - rc.left) / 2;
            int cy = rc.top + (rc.bottom - rc.top) / 2;

            int newWidth = (int)(400.0f * newSize / 56.0f);
            int newHeight = (int)(120.0f * newSize / 56.0f);

            if (newWidth < 100)  newWidth = 100;
            if (newHeight < 40)  newHeight = 40;

            int newX = cx - newWidth / 2;
            int newY = cy - newHeight / 2;

            SetWindowPos(hWnd, NULL, newX, newY, newWidth, newHeight, SWP_NOZORDER);

            g_config.width = newWidth;
            g_config.height = newHeight;
            g_config.x = newX;
            g_config.y = newY;

            Config_Save();
            UpdateLayeredWindowContent(g_hClockWnd);
        }
        return 0;
    }

    case WM_DESTROY:
        TrayIcon_Remove(hWnd);
        Config_Save();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}