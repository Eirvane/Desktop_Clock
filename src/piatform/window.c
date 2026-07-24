#include "window.h"
#include "renderer.h"
#include "config.h"
#include <windowsx.h>
#include <dwmapi.h>

/* 自动链接桌面窗口管理器API（Windows 10/11圆角效果需要） */
#pragma comment(lib, "dwmapi.lib")

/* 拖拽状态记录 */
static POINT g_dragStartPt = { 0 };
static RECT  g_dragStartRc = { 0 };

/* 前置声明：更新分层窗口内容（透明绘制核心） */
static void UpdateLayeredWindowContent(HWND hWnd);

ATOM RegisterClockWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    /* NULL_BRUSH 避免系统绘制背景，防止闪烁 */
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIconSm = LoadIconW(NULL, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

HWND CreateClockWindow(HINSTANCE hInstance)
{
    DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;

    /* 根据配置决定是否置顶 */
    if (g_config.topMost) {
        dwExStyle |= WS_EX_TOPMOST;
    }

    /*
     * WS_POPUP：创建无边框、无标题栏的弹出窗口。
     * WS_EX_LAYERED：启用分层窗口，是实现像素级透明的前提。
     * WS_EX_TOOLWINDOW：不显示在任务栏和Alt+Tab切换中。
     */
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
        /*
         * Windows 11 圆角窗口效果（DWM）。
         * 在 Windows 10 上此调用会被静默忽略，不影响兼容性。
         */
        DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_ROUND;
        DwmSetWindowAttribute(
            hWnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &cornerPref,
            sizeof(cornerPref)
        );
    }

    return hWnd;
}

/*
 * UpdateLayeredWindowContent
 * ------------------------
 * 核心透明渲染流程：
 * 1. 创建与屏幕兼容的内存DC
 * 2. 创建32位DIB Section位图（每个像素含Alpha通道）
 * 3. 将位图选入内存DC
 * 4. 调用GDI+在内存DC上绘制（背景透明 + 时间文本）
 * 5. 使用 UpdateLayeredWindow 将内存DC内容更新到窗口
 * 6. 清理GDI资源
 */
static void UpdateLayeredWindowContent(HWND hWnd)
{
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);

    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;

    /* 获取屏幕DC */
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return;

    /* 创建内存DC */
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    /*
     * 创建32位DIB Section。
     * biHeight 为负值表示自顶向下的位图（与GDI+坐标系一致）。
     * 32位 = 8位R + 8位G + 8位B + 8位Alpha。
     */
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  /* 负值：自顶向下 */
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

    /* 使用GDI+在内存DC上完成绘制 */
    Renderer_DrawClock(hdcMem, width, height);

    /*
     * 使用 UpdateLayeredWindow 更新窗口。
     * SourceConstantAlpha = 255：不使用整体Alpha，而是使用位图每个像素的Alpha。
     * AlphaFormat = AC_SRC_ALPHA：告知系统源DC包含Alpha通道。
     */
    POINT ptSrc = { 0, 0 };
    POINT ptDst = { rcWindow.left, rcWindow.top };
    SIZE  size = { width, height };
    BLENDFUNCTION blend = { 0 };

    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    /* 清理资源：按相反顺序释放 */
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        /* 窗口创建后立即绘制一次，避免初始黑屏 */
        UpdateLayeredWindowContent(hWnd);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID_UPDATE) {
            /* 每秒触发一次，更新时间显示 */
            UpdateLayeredWindowContent(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        /*
         * 左键按下：开始拖拽。
         * SetCapture 捕获鼠标消息，即使鼠标移出窗口也能接收。
         */
        SetCapture(hWnd);
        GetCursorPos(&g_dragStartPt);
        GetWindowRect(hWnd, &g_dragStartRc);
        return 0;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd) {
            POINT pt;
            GetCursorPos(&pt);

            /* 计算鼠标偏移量，同步移动窗口 */
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
        if (GetCapture() == hWnd) {
            ReleaseCapture();
            /* 拖拽结束后保存新位置 */
            RECT rc;
            GetWindowRect(hWnd, &rc);
            g_config.x = rc.left;
            g_config.y = rc.top;
        }
        return 0;

    case WM_RBUTTONUP:
    {
        /* 右键弹出极简菜单：仅提供"退出" */
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen(hWnd, &pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1, L"退出");

        int cmd = TrackPopupMenu(
            hMenu,
            TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
            pt.x, pt.y, 0, hWnd, NULL
        );

        DestroyMenu(hMenu);

        if (cmd == 1) {
            DestroyWindow(hWnd);
        }
    }
    return 0;

    case WM_DESTROY:
        /* 退出前持久化配置 */
        Config_Save();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}