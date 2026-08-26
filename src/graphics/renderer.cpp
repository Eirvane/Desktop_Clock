#include "renderer.h"
#include "config.h"
#include <gdiplus.h>
#include <wchar.h>

static ULONG_PTR g_gdiplusToken = 0;
static Gdiplus::PrivateFontCollection* g_privateFonts = NULL;
static WCHAR g_loadedFontFile[MAX_PATH] = { 0 };

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

void Renderer_DrawClock(HDC hdcDest, int width, int height)
{
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);

    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

    /* 【修改】完全透明但保留鼠标命中（Alpha=1） */
    graphics.Clear(Gdiplus::Color(1, 0, 0, 0));

    /* 【修改】仅在移动模式下绘制接近透明的提示方框 */
    if (g_config.movable) {
        float scale = g_config.fontSize / 56.0f;
        int pad = (int)(g_config.framePadding * scale);
        int bw = (int)(g_config.frameBorderWidth * scale);
        if (pad < 1) pad = 1;
        if (bw < 1)  bw = 1;

        int frameW = width - pad * 2;
        int frameH = height - pad * 2;

        if (frameW > 0 && frameH > 0) {
            Gdiplus::RectF frameRect(
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)frameW,
                (Gdiplus::REAL)frameH
            );

            /* 接近透明的填充背景（Alpha=20） */
            Gdiplus::SolidBrush fillBrush(
                Gdiplus::Color(
                    20,
                    GetRValue(g_config.frameFillColor),
                    GetGValue(g_config.frameFillColor),
                    GetBValue(g_config.frameFillColor)
                )
            );
            graphics.FillRectangle(&fillBrush, frameRect);

            /* 接近透明的边框（Alpha=40） */
            if (bw > 0) {
                Gdiplus::Pen framePen(
                    Gdiplus::Color(
                        40,
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

    /* 字体创建（PrivateFontCollection） */
    Gdiplus::Font* pFont = NULL;
    Gdiplus::FontFamily* pFamilyArray = NULL;
    BOOL fontCreated = FALSE;

    if (g_config.fontFile[0] != L'\0' && g_privateFonts != NULL) {
        if (_wcsicmp(g_loadedFontFile, g_config.fontFile) != 0) {
            delete g_privateFonts;
            g_privateFonts = new Gdiplus::PrivateFontCollection();
            g_loadedFontFile[0] = L'\0';

            Gdiplus::Status addStatus = g_privateFonts->AddFontFile(g_config.fontFile);
            if (addStatus == Gdiplus::Ok) {
                wcsncpy_s(g_loadedFontFile, MAX_PATH, g_config.fontFile, _TRUNCATE);
            }
        }

        if (g_loadedFontFile[0] != L'\0') {
            INT familyCount = g_privateFonts->GetFamilyCount();
            if (familyCount > 0) {
                pFamilyArray = new Gdiplus::FontFamily[familyCount];
                INT found = 0;
                Gdiplus::Status famStatus = g_privateFonts->GetFamilies(
                    familyCount, pFamilyArray, &found);

                if (famStatus == Gdiplus::Ok && found > 0) {
                    WCHAR realName[LF_FACESIZE] = { 0 };
                    pFamilyArray[0].GetFamilyName(realName);
                    if (realName[0] != L'\0') {
                        wcsncpy_s(g_config.fontName, 64, realName, _TRUNCATE);
                    }

                    pFont = new Gdiplus::Font(&pFamilyArray[0],
                        static_cast<Gdiplus::REAL>(g_config.fontSize),
                        Gdiplus::FontStyleRegular,
                        Gdiplus::UnitPixel);

                    if (pFont->GetLastStatus() == Gdiplus::Ok) {
                        fontCreated = TRUE;
                    }
                }
            }
        }
    }

    if (!fontCreated) {
        delete pFont;
        pFont = new Gdiplus::Font(
            g_config.fontName,
            static_cast<Gdiplus::REAL>(g_config.fontSize),
            Gdiplus::FontStyleRegular,
            Gdiplus::UnitPixel
        );
    }

    /* 画刷 */
    Gdiplus::SolidBrush brush(
        Gdiplus::Color(
            g_config.alpha,
            GetRValue(g_config.textColor),
            GetGValue(g_config.textColor),
            GetBValue(g_config.textColor)
        )
    );

    /* 文本布局：水平垂直居中 */
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF layoutRect(
        0.0f, 0.0f,
        static_cast<Gdiplus::REAL>(width),
        static_cast<Gdiplus::REAL>(height)
    );

    if (pFont && pFont->GetLastStatus() == Gdiplus::Ok) {
        graphics.DrawString(timeStr, -1, pFont, layoutRect, &format, &brush);
    }
    else {
        Gdiplus::Font fallbackFont(
            L"Microsoft YaHei UI",
            static_cast<Gdiplus::REAL>(g_config.fontSize),
            Gdiplus::FontStyleRegular,
            Gdiplus::UnitPixel
        );
        graphics.DrawString(timeStr, -1, &fallbackFont, layoutRect, &format, &brush);
    }

    delete pFont;
    delete[] pFamilyArray;

    /* 输出到目标 DC */
    Gdiplus::Graphics destGraphics(hdcDest);
    destGraphics.DrawImage(&bitmap, 0, 0);
}