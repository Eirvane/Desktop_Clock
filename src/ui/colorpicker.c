#include "colorpicker.h"
#include "../platform/window.h"
#include "../utils/config.h"
#include "../utils/utils.h"
#include <commdlg.h>
#include <stdio.h>    /* _snwprintf_s */

#pragma comment(lib, "comdlg32.lib")

/* ============================================================
 * 颜色值输入窗口 UI 配置
 * ============================================================ */
#define CVI_WINDOW_WIDTH          320
#define CVI_WINDOW_HEIGHT         180
#define CVI_WINDOW_STYLE          (WS_CAPTION | WS_SYSMENU | WS_VISIBLE)
#define CVI_WINDOW_EX_STYLE       WS_EX_DLGMODALFRAME
#define CVI_BG_COLOR_REF          (COLOR_BTNFACE + 1)
#define CVI_CORNER_RADIUS         12
#define CVI_UI_FONT_NAME          L"Microsoft YaHei"
#define CVI_UI_FONT_SIZE_PT       12
#define CVI_UI_FONT_WEIGHT        FW_NORMAL
#define CVI_EDIT_FONT_NAME        L"Consolas"
#define CVI_EDIT_FONT_SIZE_PT     12
#define CVI_EDIT_FONT_WEIGHT      FW_NORMAL
#define CVI_MARGIN_LEFT           20
#define CVI_MARGIN_TOP            22
#define CVI_LABEL_WIDTH           110
#define CVI_LABEL_HEIGHT          22
#define CVI_LABEL_X               CVI_MARGIN_LEFT
#define CVI_LABEL_Y               CVI_MARGIN_TOP
#define CVI_EDIT_X                (CVI_LABEL_X + CVI_LABEL_WIDTH + 10)
#define CVI_EDIT_Y                (CVI_MARGIN_TOP - 2)
#define CVI_EDIT_WIDTH            150
#define CVI_EDIT_HEIGHT           26
#define CVI_EDIT_MAX_CHARS        16
#define CVI_BTN_Y                 90
#define CVI_BTN_WIDTH             80
#define CVI_BTN_HEIGHT            28
#define CVI_BTN_OK_X              50
#define CVI_BTN_CANCEL_X          170
#define CVI_TEXT_OK               L"确定"
#define CVI_TEXT_CANCEL           L"取消"
#define CVI_TEXT_LABEL            L"颜色值(HEX)："
#define CVI_TEXT_TITLE            L"设置颜色值"

 /* 颜色输入窗口句柄与控件句柄（模块私有） */
static HWND  g_hColorValueWnd = NULL;
static HWND  g_hColorValueEdit = NULL;
static HWND  g_hCviLabel = NULL;
static HFONT g_hCviUiFont = NULL;
static HFONT g_hCviEditFont = NULL;

/* 前置声明 */
static LRESULT CALLBACK ColorValueInputProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* ------------------------------------------------------------------ */
/* HEX 颜色值输入窗口                                                  */
/* ------------------------------------------------------------------ */

void ColorValueInput_Show(HWND hParent)
{
    if (g_hColorValueWnd != NULL && IsWindow(g_hColorValueWnd)) {
        SetForegroundWindow(g_hColorValueWnd);
        return;
    }

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = ColorValueInputProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(CVI_BG_COLOR_REF);
    wcex.lpszClassName = L"ColorValueInputClass";
    RegisterClassExW(&wcex);

    g_hColorValueWnd = CreateWindowExW(
        CVI_WINDOW_EX_STYLE,
        L"ColorValueInputClass",
        CVI_TEXT_TITLE,
        CVI_WINDOW_STYLE,
        0, 0,
        CVI_WINDOW_WIDTH,
        CVI_WINDOW_HEIGHT,
        hParent, NULL, g_hInstance, NULL
    );

    if (g_hColorValueWnd) {
        /* 传入半径 12，不再依赖 utils.c 内部宏 */
        ApplyWindowRoundedCorners(g_hColorValueWnd, CVI_CORNER_RADIUS);
        CenterWindowOnParent(g_hColorValueWnd, hParent, CVI_WINDOW_WIDTH, CVI_WINDOW_HEIGHT);
    }
}

static LRESULT CALLBACK ColorValueInputProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
    {
        g_hCviUiFont = CreateUiFont(CVI_UI_FONT_SIZE_PT, CVI_UI_FONT_WEIGHT, CVI_UI_FONT_NAME);
        g_hCviEditFont = CreateUiFont(CVI_EDIT_FONT_SIZE_PT, CVI_EDIT_FONT_WEIGHT, CVI_EDIT_FONT_NAME);

        /* 标签 */
        g_hCviLabel = CreateWindowExW(
            0, L"STATIC", CVI_TEXT_LABEL,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            CVI_LABEL_X, CVI_LABEL_Y,
            CVI_LABEL_WIDTH, CVI_LABEL_HEIGHT,
            hWnd, NULL, g_hInstance, NULL
        );
        if (g_hCviUiFont) {
            SendMessageW(g_hCviLabel, WM_SETFONT, (WPARAM)g_hCviUiFont, TRUE);
        }

        /* 编辑框：预填当前颜色值 */
        g_hColorValueEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            CVI_EDIT_X, CVI_EDIT_Y,
            CVI_EDIT_WIDTH, CVI_EDIT_HEIGHT,
            hWnd, NULL, g_hInstance, NULL
        );
        WCHAR buf[CVI_EDIT_MAX_CHARS];
        BYTE r = GetRValue(g_config.textColor);
        BYTE g = GetGValue(g_config.textColor);
        BYTE b = GetBValue(g_config.textColor);
        _snwprintf_s(buf, CVI_EDIT_MAX_CHARS, _TRUNCATE, L"%02X%02X%02X", r, g, b);
        SetWindowTextW(g_hColorValueEdit, buf);

        if (g_hCviEditFont) {
            SendMessageW(g_hColorValueEdit, WM_SETFONT, (WPARAM)g_hCviEditFont, TRUE);
        }

        /* 确定按钮 */
        HWND hBtnOk = CreateWindowExW(
            0, L"BUTTON", CVI_TEXT_OK,
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            CVI_BTN_OK_X, CVI_BTN_Y,
            CVI_BTN_WIDTH, CVI_BTN_HEIGHT,
            hWnd, (HMENU)IDOK, g_hInstance, NULL
        );
        if (g_hCviUiFont) {
            SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)g_hCviUiFont, TRUE);
        }

        /* 取消按钮 */
        HWND hBtnCancel = CreateWindowExW(
            0, L"BUTTON", CVI_TEXT_CANCEL,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            CVI_BTN_CANCEL_X, CVI_BTN_Y,
            CVI_BTN_WIDTH, CVI_BTN_HEIGHT,
            hWnd, (HMENU)IDCANCEL, g_hInstance, NULL
        );
        if (g_hCviUiFont) {
            SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)g_hCviUiFont, TRUE);
        }
    }
    return 0;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDOK) {
            WCHAR buf[CVI_EDIT_MAX_CHARS];
            GetWindowTextW(g_hColorValueEdit, buf, CVI_EDIT_MAX_CHARS);
            COLORREF newColor = 0;
            if (TryParseHexColorString(buf, &newColor)) {
                g_config.textColor = newColor;
                Config_Save();
                UpdateLayeredWindowContent(g_hClockWnd);
            }
            DestroyWindow(hWnd);
        }
        else if (id == IDCANCEL) {
            DestroyWindow(hWnd);
        }
    }
    return 0;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        if (g_hCviUiFont) {
            DeleteObject(g_hCviUiFont);
            g_hCviUiFont = NULL;
        }
        if (g_hCviEditFont) {
            DeleteObject(g_hCviEditFont);
            g_hCviEditFont = NULL;
        }
        g_hColorValueWnd = NULL;
        g_hCviLabel = NULL;
        g_hColorValueEdit = NULL;
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

/* ------------------------------------------------------------------ */
/* 系统颜色选择对话框（ChooseColor）                                   */
/* ------------------------------------------------------------------ */

void ColorPicker_Show(HWND hParent)
{
    CHOOSECOLORW cc = { 0 };
    static COLORREF customColors[16] = { 0 };

    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hParent;
    cc.rgbResult = g_config.textColor;
    cc.lpCustColors = customColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;

    if (ChooseColorW(&cc)) {
        g_config.textColor = cc.rgbResult;
        Config_Save();
        UpdateLayeredWindowContent(g_hClockWnd);
    }
}