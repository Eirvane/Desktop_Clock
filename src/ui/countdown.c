#include "countdown.h"
#include "../platform/window.h"
#include "../utils/config.h"
#include "../utils/utils.h"
#include <wchar.h>

/* ============================================================
 * 倒计时设置窗口 UI 配置（滚动式 ListBox）
 * ============================================================ */
#define CDS_WINDOW_WIDTH     300
#define CDS_WINDOW_HEIGHT    320
#define CDS_LIST_WIDTH       60
#define CDS_LIST_HEIGHT      150
#define CDS_MARGIN_TOP       20
#define CDS_LABEL_HEIGHT     18
#define CDS_LIST_Y           45
#define CDS_BTN_WIDTH        80
#define CDS_BTN_HEIGHT       28
#define CDS_BTN_Y            220

static HWND  g_hCountdownWnd = NULL;
static HWND  g_hListHour = NULL;
static HWND  g_hListMin = NULL;
static HWND  g_hListSec = NULL;
static HFONT g_hCdsFont = NULL;

/* 向 ListBox 填充 0..maxVal，并选中 selVal */
static void InitListBox(HWND hList, int maxVal, int selVal)
{
    WCHAR buf[8];
    for (int i = 0; i <= maxVal; i++) {
        _snwprintf_s(buf, 8, _TRUNCATE, L"%02d", i);
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessageW(hList, LB_SETCURSEL, selVal, 0);
}

static LRESULT CALLBACK CountdownSettingsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
    {
        g_hCdsFont = CreateUiFont(11, FW_NORMAL, L"Microsoft YaHei");

        /* 标签：时 / 分 / 秒 */
        HWND hLblHour = CreateWindowExW(0, L"STATIC", L"时",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            30, CDS_MARGIN_TOP, CDS_LIST_WIDTH, CDS_LABEL_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);
        HWND hLblMin = CreateWindowExW(0, L"STATIC", L"分",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            110, CDS_MARGIN_TOP, CDS_LIST_WIDTH, CDS_LABEL_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);
        HWND hLblSec = CreateWindowExW(0, L"STATIC", L"秒",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            190, CDS_MARGIN_TOP, CDS_LIST_WIDTH, CDS_LABEL_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);

        /* 三个滚动列表框 */
        g_hListHour = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            30, CDS_LIST_Y, CDS_LIST_WIDTH, CDS_LIST_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);
        g_hListMin = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            110, CDS_LIST_Y, CDS_LIST_WIDTH, CDS_LIST_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);
        g_hListSec = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            190, CDS_LIST_Y, CDS_LIST_WIDTH, CDS_LIST_HEIGHT,
            hWnd, NULL, g_hInstance, NULL);

        InitListBox(g_hListHour, 23, g_config.countdownHours);
        InitListBox(g_hListMin, 59, g_config.countdownMinutes);
        InitListBox(g_hListSec, 59, g_config.countdownSeconds);

        if (g_hCdsFont) {
            SendMessageW(hLblHour, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(hLblMin, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(hLblSec, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(g_hListHour, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(g_hListMin, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(g_hListSec, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
        }

        /* 确定 / 取消 按钮 */
        HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"确定",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            50, CDS_BTN_Y, CDS_BTN_WIDTH, CDS_BTN_HEIGHT,
            hWnd, (HMENU)IDOK, g_hInstance, NULL);
        HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"取消",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            170, CDS_BTN_Y, CDS_BTN_WIDTH, CDS_BTN_HEIGHT,
            hWnd, (HMENU)IDCANCEL, g_hInstance, NULL);

        if (g_hCdsFont) {
            SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
            SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)g_hCdsFont, TRUE);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDOK) {
            int h = (int)SendMessageW(g_hListHour, LB_GETCURSEL, 0, 0);
            int m = (int)SendMessageW(g_hListMin, LB_GETCURSEL, 0, 0);
            int s = (int)SendMessageW(g_hListSec, LB_GETCURSEL, 0, 0);
            if (h == LB_ERR) h = 0;
            if (m == LB_ERR) m = 0;
            if (s == LB_ERR) s = 0;

            /* 保存设定并切换到倒计时模式 */
            g_config.countdownHours = h;
            g_config.countdownMinutes = m;
            g_config.countdownSeconds = s;
            g_config.mode = 2;

            Renderer_SetMode(2);
            Renderer_StartCountdown(h, m, s);
            Config_Save();
            UpdateLayeredWindowContent(g_hClockWnd);
            DestroyWindow(hWnd);
        }
        else if (id == IDCANCEL) {
            DestroyWindow(hWnd);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        if (g_hCdsFont) {
            DeleteObject(g_hCdsFont);
            g_hCdsFont = NULL;
        }
        g_hCountdownWnd = NULL;
        g_hListHour = NULL;
        g_hListMin = NULL;
        g_hListSec = NULL;
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

void CountdownSettings_Show(HWND hParent)
{
    if (g_hCountdownWnd != NULL && IsWindow(g_hCountdownWnd)) {
        SetForegroundWindow(g_hCountdownWnd);
        return;
    }

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = CountdownSettingsProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = L"CountdownSettingsClass";
    RegisterClassExW(&wcex);

    g_hCountdownWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"CountdownSettingsClass",
        L"设置倒计时",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, CDS_WINDOW_WIDTH, CDS_WINDOW_HEIGHT,
        hParent, NULL, g_hInstance, NULL
    );

    if (g_hCountdownWnd) {
        ApplyWindowRoundedCorners(g_hCountdownWnd, 12);
        CenterWindowOnParent(g_hCountdownWnd, hParent, CDS_WINDOW_WIDTH, CDS_WINDOW_HEIGHT);
    }
}

/* ============================================================
 * 正计时开始窗口
 * ============================================================ */
#define SSW_WINDOW_WIDTH     260
#define SSW_WINDOW_HEIGHT    160
#define SSW_BTN_WIDTH        100
#define SSW_BTN_HEIGHT       36
#define SSW_BTN_Y            60

static HWND  g_hStopwatchWnd = NULL;
static HFONT g_hSswFont = NULL;

static LRESULT CALLBACK StopwatchStartProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
    {
        g_hSswFont = CreateUiFont(12, FW_NORMAL, L"Microsoft YaHei");

        /* 提示标签 */
        HWND hLabel = CreateWindowExW(0, L"STATIC", L"点击开始按钮启动正计时",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            20, 20, SSW_WINDOW_WIDTH - 40, 24,
            hWnd, NULL, g_hInstance, NULL);
        if (g_hSswFont) {
            SendMessageW(hLabel, WM_SETFONT, (WPARAM)g_hSswFont, TRUE);
        }

        /* 开始按钮 */
        HWND hBtnStart = CreateWindowExW(0, L"BUTTON", L"开始",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            (SSW_WINDOW_WIDTH - SSW_BTN_WIDTH) / 2, SSW_BTN_Y,
            SSW_BTN_WIDTH, SSW_BTN_HEIGHT,
            hWnd, (HMENU)IDOK, g_hInstance, NULL);
        if (g_hSswFont) {
            SendMessageW(hBtnStart, WM_SETFONT, (WPARAM)g_hSswFont, TRUE);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDOK) {
            g_config.mode = 1;
            Renderer_SetMode(1);        /* 【修复】先设置模式，再启动计时 */
            Renderer_StartStopwatch();
            Config_Save();
            UpdateLayeredWindowContent(g_hClockWnd);
            DestroyWindow(hWnd);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        if (g_hSswFont) {
            DeleteObject(g_hSswFont);
            g_hSswFont = NULL;
        }
        g_hStopwatchWnd = NULL;
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

void StopwatchStart_Show(HWND hParent)
{
    if (g_hStopwatchWnd != NULL && IsWindow(g_hStopwatchWnd)) {
        SetForegroundWindow(g_hStopwatchWnd);
        return;
    }

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = StopwatchStartProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = L"StopwatchStartClass";
    RegisterClassExW(&wcex);

    g_hStopwatchWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"StopwatchStartClass",
        L"正计时",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, SSW_WINDOW_WIDTH, SSW_WINDOW_HEIGHT,
        hParent, NULL, g_hInstance, NULL
    );

    if (g_hStopwatchWnd) {
        ApplyWindowRoundedCorners(g_hStopwatchWnd, 12);
        CenterWindowOnParent(g_hStopwatchWnd, hParent, SSW_WINDOW_WIDTH, SSW_WINDOW_HEIGHT);
    }
}