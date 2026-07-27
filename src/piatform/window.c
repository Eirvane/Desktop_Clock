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

/* ============================================================
 * 颜色值输入窗口 UI 可修改数据配置区
 * 以下所有宏均可独立调整，无需改动逻辑代码
 * ============================================================ */

 /* ---- 窗口整体尺寸与位置 ---- */
#define CVI_WINDOW_WIDTH          320     /* 窗口总宽度（像素），建议范围：260~400 */
#define CVI_WINDOW_HEIGHT         180     /* 窗口总高度（像素），建议范围：140~220 */
#define CVI_WINDOW_STYLE          (WS_CAPTION | WS_SYSMENU | WS_VISIBLE) /* 窗口样式：带标题栏、系统菜单、可见；可改为 WS_DLGFRAME 等 */
#define CVI_WINDOW_EX_STYLE       WS_EX_DLGMODALFRAME                   /* 扩展样式：对话框边框，营造浮动面板感；可改为 0 或 WS_EX_TOOLWINDOW */

/* ---- 窗口圆角 ---- */
#define CVI_CORNER_RADIUS         12      /* 窗口圆角半径（像素），0 表示直角；建议 8~16 */
/* 注：Win11 下会额外通过 DWM 启用系统级圆角+阴影；Win7/10 则使用 GDI Region 圆角 */

/* ---- 通用 UI 字体（标签、按钮、标题栏） ---- */
#define CVI_UI_FONT_NAME          L"Microsoft YaHei"  /* UI 字体名称：可改为本地任意已安装字体，如 L"SimSun"、L"Segoe UI"、L"PingFang SC" 等 */
#define CVI_UI_FONT_SIZE_PT       12                    /* UI 字体字号（磅/Point），建议 9~14 */
#define CVI_UI_FONT_WEIGHT        FW_NORMAL             /* 字重：FW_NORMAL(400) 常规，FW_BOLD(700) 粗体，FW_LIGHT(300) 细体 */

/* ---- 输入框专用字体（等宽，方便对齐 HEX 字符 A-F 与数字） ---- */
#define CVI_EDIT_FONT_NAME        L"Consolas"           /* 等宽字体：也可改为 L"Courier New"、L"Lucida Console"、L"JetBrains Mono" */
#define CVI_EDIT_FONT_SIZE_PT     12                    /* 输入框字号（磅），建议与 UI 字号相同或略大 1~2pt */
#define CVI_EDIT_FONT_WEIGHT      FW_NORMAL             /* 输入框字重；如需高亮可改为 FW_BOLD */

/* ---- 控件布局（基于客户区左上角坐标，单位：像素） ---- */
#define CVI_MARGIN_LEFT           20      /* 左侧全局边距 */
#define CVI_MARGIN_TOP            22      /* 顶部全局边距 */

#define CVI_LABEL_WIDTH           110     /* "颜色值(HEX)："标签宽度 */
#define CVI_LABEL_HEIGHT          22      /* 标签高度 */
#define CVI_LABEL_X               CVI_MARGIN_LEFT
#define CVI_LABEL_Y               CVI_MARGIN_TOP

#define CVI_EDIT_X                (CVI_LABEL_X + CVI_LABEL_WIDTH + 10)  /* 输入框 X：标签右侧留 10px 间隙 */
#define CVI_EDIT_Y                (CVI_MARGIN_TOP - 2)                   /* 输入框 Y：略微上移 2px，视觉居中于标签 */
#define CVI_EDIT_WIDTH            150     /* 输入框宽度，建议 120~180 */
#define CVI_EDIT_HEIGHT           26      /* 输入框高度，建议 22~30 */
#define CVI_EDIT_MAX_CHARS        16      /* 输入框内部缓冲字符数（含终止符），需 >= 7 以容纳 "RRGGBB\0" */

/* ---- 按钮布局 ---- */
#define CVI_BTN_Y                 90      /* 按钮行纵向位置（相对于客户区顶部），建议 80~110 */
#define CVI_BTN_WIDTH             80      /* 按钮宽度，建议 70~100 */
#define CVI_BTN_HEIGHT            28      /* 按钮高度，建议 24~32 */
#define CVI_BTN_OK_X              50      /* "确定"按钮左侧 X */
#define CVI_BTN_CANCEL_X          170     /* "取消"按钮左侧 X */
#define CVI_BTN_GAP               (CVI_BTN_CANCEL_X - (CVI_BTN_OK_X + CVI_BTN_WIDTH)) /* 两按钮间隙，当前 40px */

/* ---- 按钮与标签文字（可本地化） ---- */
#define CVI_TEXT_OK               L"确定"
#define CVI_TEXT_CANCEL           L"取消"
#define CVI_TEXT_LABEL            L"颜色值(HEX)："
#define CVI_TEXT_TITLE            L"设置颜色值"

/* ============================================================
 * 全局状态
 * ============================================================ */

 /* 全局实例句柄 */
static HINSTANCE g_hInstance = NULL;

/* 时钟窗口句柄 */
static HWND g_hClockWnd = NULL;

/* 拖拽状态记录 */
static POINT g_dragStartPt = { 0 };
static RECT  g_dragStartRc = { 0 };

/* 托盘图标数据 */
static NOTIFYICONDATAW g_nid = { 0 };

/* 鼠标悬停状态：用于判断滚轮调整字号是否应当生效 */
static BOOL g_mouseHovering = FALSE;
static BOOL g_mouseTracking = FALSE;

/* 颜色值输入窗口及其控件句柄 */
static HWND g_hColorValueWnd = NULL;
static HWND g_hColorValueEdit = NULL;
static HWND g_hCviLabel = NULL;          /* 标签句柄（保存以便发送字体消息） */

/* 颜色值窗口字体资源（需在窗口销毁时释放） */
static HFONT g_hCviUiFont = NULL;        /* 微软雅黑 UI 字体 */
static HFONT g_hCviEditFont = NULL;      /* Monospace 输入框字体 */

/* 前置声明 */
static void UpdateLayeredWindowContent(HWND hWnd);
static void ColorValueInput_Show(HWND hParent);
static LRESULT CALLBACK ColorValueInputProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void ColorPicker_Show(HWND hParent);
static BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor);
static void ApplyWindowRoundedCorners(HWND hWnd);
static HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName);
static void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height);

/* ------------------------------------------------------------------ */
/* 字体与圆角辅助函数                                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief 创建指定参数的 TrueType 字体
 * @param pointSize 字号（磅）
 * @param weight    字重，如 FW_NORMAL / FW_BOLD
 * @param faceName  字体名称，如 L"Microsoft YaHei"
 * @return 成功返回 HFONT，失败返回 NULL（调用方负责 DeleteObject）
 */
static HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName)
{
    /* 将 Point 转为像素高度（负值表示字符高度，确保跨 DPI 一致性） */
    int pixelHeight = -MulDiv(pointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    return CreateFontW(
        pixelHeight,            /* 字符高度（像素） */
        0,                      /* 平均字符宽度：0 让系统自动计算 */
        0,                      /* 文本倾斜角度（0.1度单位） */
        0,                      /* 字体基线倾斜角度 */
        weight,                 /* 字重 */
        FALSE,                  /* 斜体 */
        FALSE,                  /* 下划线 */
        FALSE,                  /* 删除线 */
        DEFAULT_CHARSET,        /* 字符集：DEFAULT_CHARSET 允许系统回退到本地可用字体 */
        OUT_DEFAULT_PRECIS,     /* 输出精度 */
        CLIP_DEFAULT_PRECIS,    /* 裁剪精度 */
        CLEARTYPE_QUALITY,      /* 输出质量：ClearType 抗锯齿，使字体边缘更平滑 */
        DEFAULT_PITCH | FF_SWISS, /* 字体族：FF_SWISS 适合无衬线字体（如雅黑） */
        faceName                /* 字体名称：若本地不存在，系统会自动回退到默认字体 */
    );
}

/**
 * @brief 为窗口应用圆角效果
 * @param hWnd 目标窗口
 * @note 优先使用 DWM 原生圆角（Win10 1809+ / Win11 效果最佳），
 *       失败则回退到 GDI SetWindowRgn（兼容 Win7/8/10）。
 */
static void ApplyWindowRoundedCorners(HWND hWnd)
{
    /* 方案1：DWM 原生圆角（系统级，支持阴影和亚克力效果） */
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

    /* 方案2：GDI Region 圆角（所有 Windows 版本通用，作为后备） */
    if (CVI_CORNER_RADIUS > 0) {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        /* 窗口矩形转客户区宽高：right-left, bottom-top */
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        HRGN hRgn = CreateRoundRectRgn(0, 0, w, h, CVI_CORNER_RADIUS * 2, CVI_CORNER_RADIUS * 2);
        if (hRgn) {
            SetWindowRgn(hWnd, hRgn, TRUE);
            /* 注意：SetWindowRgn 成功后，区域所有权移交系统，禁止再 DeleteObject(hRgn) */
        }
    }
}

/**
 * @brief 将子窗口居中显示在父窗口上；无父窗口则屏幕居中
 */
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

/**
 * @brief 将十六进制颜色字符串解析为 COLORREF
 * @param hexStr 宽字符 HEX 字符串，如 L"FF5733"
 * @param outColor 接收解析后的 RGB 值
 * @return 解析成功返回 TRUE，失败返回 FALSE
 */
static BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor)
{
    unsigned int r = 0, g = 0, b = 0;
    /* 支持大小写混合的6位十六进制颜色字符串 */
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
    /* CVI_BG_COLOR_REF 未在项目中定义；使用系统默认窗口背景色作为回退值。
       (HBRUSH)(COLOR_WINDOW+1) 是为 WNDCLASSEX 指定系统颜色画笔的规范用法。 */
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"ColorValueInputClass";
    RegisterClassExW(&wcex);

    /* 创建窗口：尺寸由上方 CVI_WINDOW_WIDTH / HEIGHT 宏控制 */
    g_hColorValueWnd = CreateWindowExW(
        CVI_WINDOW_EX_STYLE,
        L"ColorValueInputClass",
        CVI_TEXT_TITLE,
        CVI_WINDOW_STYLE,
        0, 0,                       /* X,Y 临时占位，随后居中 */
        CVI_WINDOW_WIDTH,
        CVI_WINDOW_HEIGHT,
        hParent, NULL, g_hInstance, NULL
    );

    if (g_hColorValueWnd) {
        /* 应用圆角（DWM + GDI 双保险） */
        ApplyWindowRoundedCorners(g_hColorValueWnd);
        /* 在父窗口（时钟窗口）上居中；无父窗口则屏幕居中 */
        CenterWindowOnParent(g_hColorValueWnd, hParent, CVI_WINDOW_WIDTH, CVI_WINDOW_HEIGHT);
    }
}

static LRESULT CALLBACK ColorValueInputProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
    {
        /* 创建 UI 字体（微软雅黑）与输入框专用等宽字体（Consolas） */
        g_hCviUiFont = CreateUiFont(CVI_UI_FONT_SIZE_PT, CVI_UI_FONT_WEIGHT, CVI_UI_FONT_NAME);
        g_hCviEditFont = CreateUiFont(CVI_EDIT_FONT_SIZE_PT, CVI_EDIT_FONT_WEIGHT, CVI_EDIT_FONT_NAME);

        /* ---- 标签 ----
         * 可修改数据：位置(CVI_LABEL_X,CVI_LABEL_Y)、尺寸(CVI_LABEL_WIDTH,CVI_LABEL_HEIGHT)、文字(CVI_TEXT_LABEL) */
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

        /* ---- 十六进制输入框 ----
         * 可修改数据：位置(CVI_EDIT_X,CVI_EDIT_Y)、尺寸(CVI_EDIT_WIDTH,CVI_EDIT_HEIGHT)、缓冲长度(CVI_EDIT_MAX_CHARS) */
        g_hColorValueEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            CVI_EDIT_X, CVI_EDIT_Y,
            CVI_EDIT_WIDTH, CVI_EDIT_HEIGHT,
            hWnd, NULL, g_hInstance, NULL
        );
        /* 预填当前颜色的十六进制值，方便用户在此基础上微调 */
        WCHAR buf[CVI_EDIT_MAX_CHARS];
        BYTE r = GetRValue(g_config.textColor);
        BYTE g = GetGValue(g_config.textColor);
        BYTE b = GetBValue(g_config.textColor);
        _snwprintf_s(buf, CVI_EDIT_MAX_CHARS, _TRUNCATE, L"%02X%02X%02X", r, g, b);
        SetWindowTextW(g_hColorValueEdit, buf);

        /* 为输入框应用 Monospace 等宽字体，使 HEX 字符宽度一致，提升可读性 */
        if (g_hCviEditFont) {
            SendMessageW(g_hColorValueEdit, WM_SETFONT, (WPARAM)g_hCviEditFont, TRUE);
        }

        /* ---- 确定按钮 ----
         * 可修改数据：位置(CVI_BTN_OK_X,CVI_BTN_Y)、尺寸(CVI_BTN_WIDTH,CVI_BTN_HEIGHT)、文字(CVI_TEXT_OK) */
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

        /* ---- 取消按钮 ----
         * 可修改数据：位置(CVI_BTN_CANCEL_X,CVI_BTN_Y)、尺寸(CVI_BTN_WIDTH,CVI_BTN_HEIGHT)、文字(CVI_TEXT_CANCEL) */
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
            /* 解析并应用颜色；若格式错误则静默忽略，保持原色 */
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
        /* 销毁窗口时释放创建的字体资源，防止 GDI 句柄泄漏 */
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
    cc.hwndOwner = hParent;  /* 父窗口设为时钟窗口，对话框会出现在其附近 */
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
            /* 构建右键菜单：颜色改为二级子菜单，彻底移除字号入口 */
            HMENU hMenu = CreatePopupMenu();

            /* 创建"颜色"子菜单，仅保留两个自定义选项 */
            HMENU hColorMenu = CreatePopupMenu();
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_VALUE, L"颜色值");
            AppendMenuW(hColorMenu, MF_STRING, ID_MENU_COLOR_PANEL, L"颜色面板");

            /* 将颜色子菜单作为弹出菜单挂载到主菜单 */
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
                /* 弹出十六进制颜色值输入框，手动精确指定颜色 */
                ColorValueInput_Show(g_hClockWnd);
            }
            else if (cmd == ID_MENU_COLOR_PANEL) {
                /* 弹出系统颜色选择对话框，可视化取色 */
                ColorPicker_Show(g_hClockWnd);
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

        /* 注册鼠标离开通知：首次进入窗口客户区时启用 TrackMouseEvent，
         * 这样即使时钟窗口没有键盘焦点，只要鼠标悬停其上，
         * 后续滚轮消息即可被识别为"悬停状态"。 */
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
        /* 鼠标已离开时钟区域，重置悬停标志。
         * 此后滚轮消息将不再触发字号调整，防止后台误触。 */
        g_mouseHovering = FALSE;
        g_mouseTracking = FALSE;
        return 0;

    case WM_MOUSEWHEEL:
    {
        /* 严格校验鼠标是否处于悬停状态：只有指针真正位于时钟窗口上方时才响应，
         * 避免窗口失焦或在后台时误触字号调整。 */
        if (!g_mouseHovering) {
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }

        /* 获取滚轮方向：正值为向前滚动（远离用户），负值为向后滚动（靠近用户） */
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        /* 每格滚轮变化 2 像素，兼顾微调精度与操作效率 */
        int step = (zDelta > 0) ? 2 : -2;
        int newSize = g_config.fontSize + step;

        /* 限制字号边界，防止渲染异常（过小无法阅读，过大超出窗口） */
        if (newSize < 8) newSize = 8;
        if (newSize > 200) newSize = 200;

        if (newSize != g_config.fontSize) {
            g_config.fontSize = newSize;
            Config_Save();                     /* 立即持久化到 INI */
            UpdateLayeredWindowContent(g_hClockWnd); /* 实时重绘窗口 */
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