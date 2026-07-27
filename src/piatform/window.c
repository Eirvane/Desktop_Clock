#include "window.h"
#include "renderer.h"
#include "config.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdio.h>

/* 自动链接所需的系统库 */
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")   /* AddFontResourceExW 需要 */

/* ============================================================
 * 颜色值输入窗口 UI 可修改数据配置区
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

 /* ============================================================
  * 字体菜单配置区
  * ============================================================ */
#define FONT_FOLDER_PATH    L"D:\\C++\\Desktop_Clock\\fonts"
#define FONT_FILE_FILTER    L"D:\\C++\\Desktop_Clock\\fonts\\*"

  /* 字体菜单项结构：保存菜单命令ID与字体文件信息 */
typedef struct {
    UINT  id;                      /* 菜单命令ID，范围 ID_MENU_FONT_BASE ~ ID_MENU_FONT_MAX */
    WCHAR fileName[MAX_PATH];      /* 完整文件名，如 L"CustomFont.ttf" */
    WCHAR displayName[64];         /* 菜单显示名（去掉扩展名，下划线转空格） */
} FontMenuItem;

/* 扫描结果缓存：避免每次弹出菜单都重复读取磁盘 */
static FontMenuItem g_fontMenuItems[MAX_FONT_MENU_ITEMS];
static int g_fontMenuItemCount = 0;

/* ============================================================
 * 全局状态
 * ============================================================ */
static HINSTANCE g_hInstance = NULL;
static HWND g_hClockWnd = NULL;
static POINT g_dragStartPt = { 0 };
static RECT  g_dragStartRc = { 0 };
static NOTIFYICONDATAW g_nid = { 0 };
static BOOL g_mouseHovering = FALSE;
static BOOL g_mouseTracking = FALSE;
static HWND g_hColorValueWnd = NULL;
static HWND g_hColorValueEdit = NULL;
static HWND g_hCviLabel = NULL;
static HFONT g_hCviUiFont = NULL;
static HFONT g_hCviEditFont = NULL;

/* 前置声明 */
static void UpdateLayeredWindowContent(HWND hWnd);
static void ColorValueInput_Show(HWND hParent);
static LRESULT CALLBACK ColorValueInputProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void ColorPicker_Show(HWND hParent);
static BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor);
static void ApplyWindowRoundedCorners(HWND hWnd);
static HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName);
static void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height);

/* 字体菜单相关前置声明 */
static BOOL IsFontFileExtension(const WCHAR* ext);
static void ExtractFontDisplayName(const WCHAR* filePath, WCHAR* outName, int outSize);
static void ScanFontsFolder(void);
static HMENU BuildFontSubmenu(void);
static void HandleFontMenuCommand(UINT cmdId);

/* ------------------------------------------------------------------ */
/* 字体菜单辅助函数                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief 检查文件扩展名是否为支持的字体格式
 * @param ext 扩展名（包含点），如 L".ttf"
 * @return 支持返回 TRUE
 */
static BOOL IsFontFileExtension(const WCHAR* ext)
{
    if (!ext) return FALSE;
    return (_wcsicmp(ext, L".ttf") == 0 ||
        _wcsicmp(ext, L".otf") == 0 ||
        _wcsicmp(ext, L".ttc") == 0 ||
        _wcsicmp(ext, L".fon") == 0);
}

/**
 * @brief 从完整路径中提取文件名（不含扩展名）作为菜单显示名
 * @param filePath 完整路径，如 L"D:\\...\\Microsoft_YaHei.ttf"
 * @param outName  输出缓冲
 * @param outSize  输出缓冲大小（字符数）
 * @note 同时将下划线替换为空格，使文件名更接近真实字体家族名
 */
static void ExtractFontDisplayName(const WCHAR* filePath, WCHAR* outName, int outSize)
{
    const WCHAR* nameStart = wcsrchr(filePath, L'\\');
    if (!nameStart) nameStart = filePath;
    else nameStart++; /* 跳过反斜杠 */

    const WCHAR* ext = wcsrchr(nameStart, L'.');
    size_t len = ext ? (size_t)(ext - nameStart) : wcslen(nameStart);
    if (len >= (size_t)outSize) len = (size_t)outSize - 1;

    wcsncpy_s(outName, outSize, nameStart, len);
    outName[len] = L'\0';

    /* 将下划线替换为空格：例如 "Microsoft_YaHei" -> "Microsoft YaHei" */
    for (int i = 0; outName[i]; i++) {
        if (outName[i] == L'_') outName[i] = L' ';
    }
}

/**
 * @brief 扫描字体目录，将支持的字体文件填入全局缓存数组
 * @note 每次调用会清空旧缓存；若目录不存在或为空，则 count 为 0
 */
static void ScanFontsFolder(void)
{
    g_fontMenuItemCount = 0;
    ZeroMemory(g_fontMenuItems, sizeof(g_fontMenuItems));

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(FONT_FILE_FILTER, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        const WCHAR* ext = wcsrchr(findData.cFileName, L'.');
        if (!IsFontFileExtension(ext)) continue;

        if (g_fontMenuItemCount >= MAX_FONT_MENU_ITEMS) break;

        FontMenuItem* item = &g_fontMenuItems[g_fontMenuItemCount];
        item->id = ID_MENU_FONT_BASE + g_fontMenuItemCount;
        wcsncpy_s(item->fileName, MAX_PATH, findData.cFileName, _TRUNCATE);
        ExtractFontDisplayName(findData.cFileName, item->displayName, 64);

        g_fontMenuItemCount++;
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

/**
 * @brief 构建字体子菜单（动态扫描目录）
 * @return 返回子菜单句柄；失败返回 NULL
 * @note 返回的 HMENU 所有权将移交给父菜单，调用方无需单独销毁
 */
static HMENU BuildFontSubmenu(void)
{
    HMENU hFontMenu = CreatePopupMenu();
    if (!hFontMenu) return NULL;

    /* 每次构建前重新扫描，确保文件增删能及时反映 */
    ScanFontsFolder();

    if (g_fontMenuItemCount == 0) {
        AppendMenuW(hFontMenu, MF_STRING | MF_GRAYED, 0, L"(无字体文件)");
    }
    else {
        for (int i = 0; i < g_fontMenuItemCount; i++) {
            UINT flags = MF_STRING;
            /* 若当前配置字体名与该项匹配，则打勾标记 */
            if (_wcsicmp(g_config.fontName, g_fontMenuItems[i].displayName) == 0) {
                flags |= MF_CHECKED;
            }
            AppendMenuW(hFontMenu, flags, g_fontMenuItems[i].id, g_fontMenuItems[i].displayName);
        }
    }

    return hFontMenu;
}

/**
 * @brief 处理字体菜单项点击：加载字体文件并应用
 * @param cmdId 菜单命令ID
 * @note 使用 AddFontResourceExW(FR_PRIVATE) 将字体临时加载到当前进程，
 *       无需安装到系统；进程退出后自动释放。
 */
static void HandleFontMenuCommand(UINT cmdId)
{
    int index = (int)(cmdId - ID_MENU_FONT_BASE);
    if (index < 0 || index >= g_fontMenuItemCount) return;

    /* 构造完整文件路径 */
    WCHAR filePath[MAX_PATH];
    _snwprintf_s(filePath, MAX_PATH, _TRUNCATE,
        L"%s\\%s", FONT_FOLDER_PATH, g_fontMenuItems[index].fileName);

    /**
     * 将字体文件加载到当前进程（不写入系统字体库）。
     * FR_PRIVATE：仅当前进程可见，进程退出自动卸载；
     * 返回 0 表示加载失败（可能文件损坏或路径错误），但仍继续尝试设置字体名。
     */
    int added = AddFontResourceExW(filePath, FR_PRIVATE, 0);
    (void)added; /* 静默处理失败，GDI+ 可能已通过其他途径识别该字体 */

    /* 将处理后的显示名（下划线已转空格）作为字体家族名写入配置 */
    wcsncpy_s(g_config.fontName, 64, g_fontMenuItems[index].displayName, _TRUNCATE);
    Config_Save();
    UpdateLayeredWindowContent(g_hClockWnd);
}

/* ------------------------------------------------------------------ */
/* 字体与圆角辅助函数                                                  */
/* ------------------------------------------------------------------ */

static HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName)
{
    int pixelHeight = -MulDiv(pointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    return CreateFontW(
        pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, faceName
    );
}

static void ApplyWindowRoundedCorners(HWND hWnd)
{
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
        DwmSetWindowAttributeFn pDwmSetWindowAttribute =
            (DwmSetWindowAttributeFn)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pDwmSetWindowAttribute) {
            DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
            pDwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
        FreeLibrary(hDwm);
    }

    if (CVI_CORNER_RADIUS > 0) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        HRGN hRgn = CreateRoundRectRgn(0, 0, w, h, CVI_CORNER_RADIUS * 2, CVI_CORNER_RADIUS * 2);
        if (hRgn) {
            SetWindowRgn(hWnd, hRgn, TRUE);
        }
    }
}

static void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height)
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
/* 时钟窗口相关                                                        */
/* ------------------------------------------------------------------ */

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
        DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_ROUND;
        DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
    }

    return hWnd;
}

/* ------------------------------------------------------------------ */
/* 系统托盘相关                                                        */
/* ------------------------------------------------------------------ */

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
    g_nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, 128, L"桌面时钟");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void TrayIcon_Remove(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

/* ------------------------------------------------------------------ */
/* 颜色值输入窗口（极简弹窗）                                          */
/* ------------------------------------------------------------------ */

static BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor)
{
    unsigned int r = 0, g = 0, b = 0;
    if (swscanf_s(hexStr, L"%2x%2x%2x", &r, &g, &b) == 3) {
        *outColor = RGB(r, g, b);
        return TRUE;
    }
    return FALSE;
}

static void ColorValueInput_Show(HWND hParent)
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
        ApplyWindowRoundedCorners(g_hColorValueWnd);
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
/* 颜色选择（以时钟窗口为父窗口，出现在窗口旁）                         */
/* ------------------------------------------------------------------ */

static void ColorPicker_Show(HWND hParent)
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

/* ------------------------------------------------------------------ */
/* 时钟渲染（与之前一致）                                               */
/* ------------------------------------------------------------------ */

static void UpdateLayeredWindowContent(HWND hWnd)
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

/* ------------------------------------------------------------------ */
/* 主窗口过程                                                          */
/* ------------------------------------------------------------------ */

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
            /* 构建右键菜单：字体 -> [扫描目录]；颜色 -> [颜色值, 颜色面板]；退出 */
            HMENU hMenu = CreatePopupMenu();

            /* ---- 字体子菜单 ---- */
            HMENU hFontMenu = BuildFontSubmenu();
            if (hFontMenu) {
                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFontMenu, L"字体");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            }

            /* ---- 颜色子菜单 ---- */
            HMENU hColorMenu = CreatePopupMenu();
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_VALUE, L"颜色值");
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_PANEL, L"颜色面板");

            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hColorMenu, L"颜色");
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
            else if (cmd >= ID_MENU_FONT_BASE && cmd <= ID_MENU_FONT_MAX) {
                /* 命中动态字体菜单项，加载并应用对应字体文件 */
                HandleFontMenuCommand(cmd);
            }
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hWnd);
        GetCursorPos(&g_dragStartPt);
        GetWindowRect(hWnd, &g_dragStartRc);
        return 0;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd) {
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

        if (!g_mouseTracking) {
            TRACKMOUSEEVENT tme = { 0 };
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            g_mouseTracking = TRUE;
            g_mouseHovering = TRUE;
        }
        return 0;

    case WM_MOUSELEAVE:
        g_mouseHovering = FALSE;
        g_mouseTracking = FALSE;
        return 0;

    case WM_MOUSEWHEEL:
    {
        if (!g_mouseHovering) {
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }

        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int step = (zDelta > 0) ? 2 : -2;
        int newSize = g_config.fontSize + step;

        if (newSize < 8) newSize = 8;
        if (newSize > 200) newSize = 200;

        if (newSize != g_config.fontSize) {
            g_config.fontSize = newSize;
            Config_Save();
            UpdateLayeredWindowContent(g_hClockWnd);
        }
        return 0;
    }

    case WM_LBUTTONUP:
        if (GetCapture() == hWnd) {
            ReleaseCapture();
            RECT rc;
            GetWindowRect(hWnd, &rc);
            g_config.x = rc.left;
            g_config.y = rc.top;
        }
        return 0;

    case WM_DESTROY:
        TrayIcon_Remove(hWnd);
        Config_Save();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}