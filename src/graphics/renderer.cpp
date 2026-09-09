#include "renderer.h"
#include "config.h"
#include <gdiplus.h>
#include <wchar.h>

static ULONG_PTR g_gdiplusToken = 0;
static Gdiplus::PrivateFontCollection* g_privateFonts = NULL;
static WCHAR g_loadedFontFile[MAX_PATH] = { 0 };

// 模式与计时状态
static int       g_mode = 0;
static BOOL      g_stopwatchRunning = FALSE;
static ULONGLONG g_stopwatchStart = 0;
static ULONGLONG g_countdownEnd = 0;

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

/* C 接口实现 */
extern "C" void Renderer_SetMode(int mode)
{
    g_mode = mode;
    if (mode == 1) {
        g_stopwatchRunning = FALSE;   
    }
}

extern "C" void Renderer_StartStopwatch(void)
{
    g_stopwatchRunning = TRUE;
    g_stopwatchStart = GetTickCount64();
}

extern "C" void Renderer_StartCountdown(int hours, int minutes, int seconds)
{
    ULONGLONG totalMs = ((ULONGLONG)hours * 3600ULL
        + (ULONGLONG)minutes * 60ULL
        + (ULONGLONG)seconds) * 1000ULL;
    g_countdownEnd = GetTickCount64() + totalMs;
}

static void GetDisplayString(WCHAR* buffer, int bufferSize)
{
    if (g_mode == 0) {
        // 当前时间 
        SYSTEMTIME st;
        GetLocalTime(&st);

        if (g_config.hourFormat == 1) {
            int hour12 = st.wHour % 12;
            if (hour12 == 0) hour12 = 12;

            if (g_config.showSeconds) {
                swprintf_s(buffer, bufferSize, L"%d:%02d:%02d",
                    hour12, st.wMinute, st.wSecond);
            }
            else {
                swprintf_s(buffer, bufferSize, L"%d:%02d",
                    hour12, st.wMinute);
            }
        }
        else {
            if (g_config.showSeconds) {
                swprintf_s(buffer, bufferSize, L"%02d:%02d:%02d",
                    st.wHour, st.wMinute, st.wSecond);
            }
            else {
                swprintf_s(buffer, bufferSize, L"%02d:%02d",
                    st.wHour, st.wMinute);
            }
        }
    }
    else if (g_mode == 1) {
        // 正计时 
        if (!g_stopwatchRunning) {
            swprintf_s(buffer, bufferSize, L"00:00:00");
        }
        else {
            ULONGLONG elapsed = GetTickCount64() - g_stopwatchStart;
            int hours = (int)(elapsed / 3600000ULL);
            int mins = (int)((elapsed % 3600000ULL) / 60000ULL);
            int secs = (int)((elapsed % 60000ULL) / 1000ULL);
            swprintf_s(buffer, bufferSize, L"%02d:%02d:%02d", hours, mins, secs);
        }
    }
    else if (g_mode == 2) {
        // 倒计时 
        ULONGLONG now = GetTickCount64();
        if (now >= g_countdownEnd) {
            swprintf_s(buffer, bufferSize, L"00:00:00");
        }
        else {
            ULONGLONG remaining = g_countdownEnd - now;
            int hours = (int)(remaining / 3600000ULL);
            int mins = (int)((remaining % 3600000ULL) / 60000ULL);
            int secs = (int)((remaining % 60000ULL) / 1000ULL);
            swprintf_s(buffer, bufferSize, L"%02d:%02d:%02d", hours, mins, secs);
        }
    }
}

void Renderer_DrawClock(HDC hdcDest, int width, int height)
{
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);

    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

    graphics.Clear(Gdiplus::Color(1, 0, 0, 0));

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

            Gdiplus::SolidBrush fillBrush(
                Gdiplus::Color(
                    20,
                    GetRValue(g_config.frameFillColor),
                    GetGValue(g_config.frameFillColor),
                    GetBValue(g_config.frameFillColor)
                )
            );
            graphics.FillRectangle(&fillBrush, frameRect);

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

    WCHAR timeStr[32];
    GetDisplayString(timeStr, 32);

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

    Gdiplus::Graphics destGraphics(hdcDest);
    destGraphics.DrawImage(&bitmap, 0, 0);
}