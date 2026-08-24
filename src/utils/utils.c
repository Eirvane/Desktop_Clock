#include "utils.h"
#include <wchar.h>
#include <dwmapi.h>   /* DwmSetWindowAttribute 等 */

/* ------------------------------------------------------------------ */
/* 字体辅助                                                            */
/* ------------------------------------------------------------------ */

HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName)
{
    int pixelHeight = -MulDiv(pointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    return CreateFontW(
        pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, faceName
    );
}

/* ------------------------------------------------------------------ */
/* 窗口圆角（DWM + HRGN 双保险）                                       */
/* ------------------------------------------------------------------ */

void ApplyWindowRoundedCorners(HWND hWnd, int radius)
{
    /* 方式1：DWM 原生圆角（Win11 / Win10 20H1+，效果最好） */
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
        DwmSetWindowAttributeFn pDwmSetWindowAttribute =
            (DwmSetWindowAttributeFn)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pDwmSetWindowAttribute) {
            int corner = DWMWCP_ROUND;
            pDwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
        FreeLibrary(hDwm);
    }

    /* 方式2：HRGN 圆角（兼容性回退） */
    if (radius > 0) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        HRGN hRgn = CreateRoundRectRgn(0, 0, w, h, radius * 2, radius * 2);
        if (hRgn) {
            SetWindowRgn(hWnd, hRgn, TRUE);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 窗口居中                                                            */
/* ------------------------------------------------------------------ */

void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height)
{
    int x, y;
    if (hParent && IsWindow(hParent)) {
        RECT rc;
        GetWindowRect(hParent, &rc);
        x = rc.left + ((rc.right - rc.left) - width) / 2;
        y = rc.top + ((rc.bottom - rc.top) - height) / 2;
    }
    else {
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        x = (sw - width) / 2;
        y = (sh - height) / 2;
    }
    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* ------------------------------------------------------------------ */
/* HEX 颜色解析                                                        */
/* ------------------------------------------------------------------ */

BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor)
{
    unsigned int r = 0, g = 0, b = 0;
    if (swscanf_s(hexStr, L"%2x%2x%2x", &r, &g, &b) == 3) {
        *outColor = RGB(r, g, b);
        return TRUE;
    }
    return FALSE;
}