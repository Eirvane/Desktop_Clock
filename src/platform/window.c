#include "window.h"
#include "../graphics/renderer.h"
#include "../utils/config.h"
#include "../utils/utils.h"
#include "trayicon.h"
#include "../ui/fontmenu.h"
#include "../ui/colorpicker.h"
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

HINSTANCE g_hInstance = NULL;
HWND      g_hClockWnd = NULL;

static POINT g_dragStartPt = { 0 };
static RECT  g_dragStartRc = { 0 };

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
    /* 【关键】已固定状态：添加 WS_EX_TRANSPARENT，鼠标穿透到下层窗口 */
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
        /* 移动模式启用圆角，固定模式禁用（避免实线边缘） */
        int cornerPref = g_config.movable ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
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

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();

            HMENU hFontMenu = BuildFontSubmenu();
            if (hFontMenu) {
                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFontMenu, L"字体");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            }

            HMENU hColorMenu = CreatePopupMenu();
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_VALUE, L"颜色值");
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_PANEL, L"颜色面板");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hColorMenu, L"颜色");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            /* 【修改】菜单显示当前状态：正在移动 / 已固定 */
            WCHAR toggleText[32];
            if (g_config.movable) {
                wcscpy_s(toggleText, 32, L"正在移动");
            }
            else {
                wcscpy_s(toggleText, 32, L"已固定");
            }
            AppendMenuW(hMenu, MF_STRING, ID_MENU_TOGGLE_MOVE, toggleText);
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"退出");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);

            int cmd = TrackPopupMenu(
                hMenu,
                TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                pt.x, pt.y, 0, hWnd, NULL
            );

            DestroyMenu(hMenu);

            if (cmd == ID_MENU_EXIT) {
                DestroyWindow(hWnd);
            }
            else if (cmd == ID_MENU_COLOR_VALUE) {
                ColorValueInput_Show(g_hClockWnd);
            }
            else if (cmd == ID_MENU_COLOR_PANEL) {
                ColorPicker_Show(g_hClockWnd);
            }
            else if (cmd == ID_MENU_TOGGLE_MOVE) {
                g_config.movable = !g_config.movable;
                Config_Save();

                /* 【关键】动态修改窗口扩展样式：切换鼠标穿透状态 */
                LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
                if (g_config.movable) {
                    /* 切换到移动模式：移除透明，开始接收鼠标 */
                    exStyle &= ~WS_EX_TRANSPARENT;
                }
                else {
                    /* 切换到固定模式：添加透明，鼠标穿透到下层 */
                    exStyle |= WS_EX_TRANSPARENT;
                }
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);

                /* 同步更新 DWM 圆角 */
                int cornerPref = g_config.movable ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
                DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

                /* 强制刷新窗口样式（必须带 SWP_FRAMECHANGED） */
                SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

                UpdateLayeredWindowContent(g_hClockWnd);
            }
            else if (cmd >= ID_MENU_FONT_BASE && cmd <= ID_MENU_FONT_MAX) {
                HandleFontMenuCommand(cmd);
            }
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        /* 仅在移动模式下允许拖拽 */
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
        /* 固定状态下：WS_EX_TRANSPARENT 已使鼠标穿透，本消息不会进入。
         * 保留判断作为保险。 */
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

            /* 窗口尺寸联动缩放 */
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