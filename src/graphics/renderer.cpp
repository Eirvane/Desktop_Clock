#include "renderer.h"
#include "config.h"
#include <gdiplus.h>
#include <wchar.h>

/* GDI+启动令牌 */
static ULONG_PTR g_gdiplusToken = 0;

/* 私有字体集合：直接从文件加载，不依赖系统字体名称 */
static Gdiplus::PrivateFontCollection* g_privateFonts = NULL;

/* 记录当前已加载的字体文件路径，避免重复加载 */
static WCHAR g_loadedFontFile[MAX_PATH] = { 0 };

/* 初始化GDI+ */
BOOL Renderer_Init(void)
{
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartupOutput output;
    Gdiplus::Status status;

    memset(&input, 0, sizeof(input));
    input.GdiplusVersion = 1;
    memset(&output, 0, sizeof(output));

    status = Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, &output);

    if (status == Gdiplus::Ok) {
        g_privateFonts = new Gdiplus::PrivateFontCollection();
    }

    return (status == Gdiplus::Ok);
}

/* 关闭GDI+ */
void Renderer_Shutdown(void)
{
    if (g_privateFonts) {
        delete g_privateFonts;
        g_privateFonts = NULL;
    }
    g_loadedFontFile[0] = L'\0';

    if (g_gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

/* 获取当前本地时间字符串 */
static void GetTimeString(WCHAR* buffer, int bufferSize)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    if (g_config.showSeconds) {
        swprintf_s(buffer, bufferSize, L"%02d:%02d:%02d",
            st.wHour, st.wMinute, st.wSecond);
    }
    else {
        swprintf_s(buffer, bufferSize, L"%02d:%02d",
            st.wHour, st.wMinute);
    }
}

/* ------------------------------------------------------------------ */
/* 核心绘制函数                                                       */
/* ------------------------------------------------------------------ */

void Renderer_DrawClock(HDC hdcDest, int width, int height)
{
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);

    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

    /* 绘制半透明方框 */
    if (g_config.showFrame) {
        int pad = g_config.framePadding;
        int bw = g_config.frameBorderWidth;

        int frameW = width - pad * 2;
        int frameH = height - pad * 2;
        if (frameW > 0 && frameH > 0) {
            Gdiplus::RectF frameRect(
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)frameW,
                (Gdiplus::REAL)frameH
            );

            Gdiplus::SolidBrush fillBrush(
                Gdiplus::Color(
                    g_config.frameAlpha,
                    GetRValue(g_config.frameFillColor),
                    GetGValue(g_config.frameFillColor),
                    GetBValue(g_config.frameFillColor)
                )
            );
            graphics.FillRectangle(&fillBrush, frameRect);

            if (bw > 0) {
                Gdiplus::Pen framePen(
                    Gdiplus::Color(
                        g_config.alpha,
                        GetRValue(g_config.frameColor),
                        GetGValue(g_config.frameColor),
                        GetBValue(g_config.frameColor)
                    ),
                    (Gdiplus::REAL)bw
                );
                Gdiplus::RectF borderRect = frameRect;
                borderRect.X += bw / 2.0f;
                borderRect.Y += bw / 2.0f;
                borderRect.Width -= bw;
                borderRect.Height -= bw;

                if (borderRect.Width > 0 && borderRect.Height > 0) {
                    graphics.DrawRectangle(&framePen, borderRect);
                }
            }
        }
    }

    /* 准备时间文本 */
    WCHAR timeStr[32];
    GetTimeString(timeStr, 32);

    /* ============================================================
     * 字体创建（关键修复）
     * ============================================================ */
    Gdiplus::Font* pFont = NULL;
    Gdiplus::FontFamily* pFamilyArray = NULL;
    BOOL fontCreated = FALSE;

    /* --- 调试输出：查看 g_config.fontFile 实际值 --- */
    OutputDebugStringW(L"[DesktopClock] fontFile = ");
    OutputDebugStringW(g_config.fontFile[0] ? g_config.fontFile : L"(empty)");
    OutputDebugStringW(L"\n");

    /* 若配置了字体文件路径，尝试用 PrivateFontCollection 直接加载 */
    if (g_config.fontFile[0] != L'\0' && g_privateFonts != NULL) {

        /* 路径变化时重建集合（PrivateFontCollection 不支持动态移除） */
        if (_wcsicmp(g_loadedFontFile, g_config.fontFile) != 0) {
            delete g_privateFonts;
            g_privateFonts = new Gdiplus::PrivateFontCollection();
            g_loadedFontFile[0] = L'\0';

            Gdiplus::Status addStatus = g_privateFonts->AddFontFile(g_config.fontFile);

            /* 调试输出 */
            WCHAR dbg[256];
            swprintf_s(dbg, L"[DesktopClock] AddFontFile status = %d (Ok=%d)\n",
                (int)addStatus, (int)Gdiplus::Ok);
            OutputDebugStringW(dbg);

            if (addStatus == Gdiplus::Ok) {
                wcsncpy_s(g_loadedFontFile, MAX_PATH, g_config.fontFile, _TRUNCATE);
            }
        }

        /* 从私有集合中提取字体家族 */
        if (g_loadedFontFile[0] != L'\0') {
            INT familyCount = g_privateFonts->GetFamilyCount();

            WCHAR dbg[256];
            swprintf_s(dbg, L"[DesktopClock] GetFamilyCount = %d\n", familyCount);
            OutputDebugStringW(dbg);

            if (familyCount > 0) {
                pFamilyArray = new Gdiplus::FontFamily[familyCount];
                INT found = 0;
                Gdiplus::Status famStatus = g_privateFonts->GetFamilies(
                    familyCount, pFamilyArray, &found);

                swprintf_s(dbg, L"[DesktopClock] GetFamilies found = %d, status = %d\n",
                    found, (int)famStatus);
                OutputDebugStringW(dbg);

                if (famStatus == Gdiplus::Ok && found > 0) {
                    /* 提取真实家族名，更新到配置（供系统回退路径使用） */
                    WCHAR realName[LF_FACESIZE] = { 0 };
                    pFamilyArray[0].GetFamilyName(realName);
                    if (realName[0] != L'\0') {
                        wcsncpy_s(g_config.fontName, 64, realName, _TRUNCATE);
                    }

                    swprintf_s(dbg, L"[DesktopClock] Real family name = %s\n", realName);
                    OutputDebugStringW(dbg);

                    pFont = new Gdiplus::Font(&pFamilyArray[0],
                        static_cast<Gdiplus::REAL>(g_config.fontSize),
                        Gdiplus::FontStyleRegular,
                        Gdiplus::UnitPixel);

                    if (pFont->GetLastStatus() == Gdiplus::Ok) {
                        fontCreated = TRUE;
                        OutputDebugStringW(L"[DesktopClock] PrivateFont Font created OK\n");
                    }
                    else {
                        OutputDebugStringW(L"[DesktopClock] PrivateFont Font creation FAILED\n");
                    }
                }
            }
        }
    }

    /* 私有字体路径失败时，回退到系统字体查找 */
    if (!fontCreated) {
        delete pFont;
        pFont = new Gdiplus::Font(
            g_config.fontName,
            static_cast<Gdiplus::REAL>(g_config.fontSize),
            Gdiplus::FontStyleRegular,
            Gdiplus::UnitPixel
        );
        OutputDebugStringW(L"[DesktopClock] Fallback to system font name\n");
    }

    /* 画刷与布局 */
    Gdiplus::SolidBrush brush(
        Gdiplus::Color(
            g_config.alpha,
            GetRValue(g_config.textColor),
            GetGValue(g_config.textColor),
            GetBValue(g_config.textColor)
        )
    );

    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF layoutRect(
        0.0f, 0.0f,
        static_cast<Gdiplus::REAL>(width),
        static_cast<Gdiplus::REAL>(height)
    );

    /* 绘制 */
    if (pFont && pFont->GetLastStatus() == Gdiplus::Ok) {
        graphics.DrawString(timeStr, -1, pFont, layoutRect, &format, &brush);
    }
    else {
        /* 最终保险：用默认字体 */
        Gdiplus::Font fallbackFont(
            L"Microsoft YaHei UI",
            static_cast<Gdiplus::REAL>(g_config.fontSize),
            Gdiplus::FontStyleRegular,
            Gdiplus::UnitPixel
        );
        graphics.DrawString(timeStr, -1, &fallbackFont, layoutRect, &format, &brush);
    }

    /* 清理：先删 Font，再删 FontFamily 数组（Font 内部已复制数据，顺序安全） */
    delete pFont;
    delete[] pFamilyArray;

    /* 输出到目标 DC */
    Gdiplus::Graphics destGraphics(hdcDest);
    destGraphics.DrawImage(&bitmap, 0, 0);
}