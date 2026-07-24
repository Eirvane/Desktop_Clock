#include "renderer.h"
#include "config.h"
#include <gdiplus.h>
#include <wchar.h>

/* GDI+启动令牌 */
static ULONG_PTR g_gdiplusToken = 0;

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
    return (status == Gdiplus::Ok);
}

/* 关闭GDI+ */
void Renderer_Shutdown(void)
{
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

/* 核心绘制函数：在内存DC上绘制透明背景的时间文本 */
void Renderer_DrawClock(HDC hdcDest, int width, int height)
{
    /*
     * 创建与目标DC兼容的内存Bitmap，格式为32位ARGB。
     * PixelFormat32bppARGB 确保每个像素都有独立的Alpha通道，
     * 这是 UpdateLayeredWindow 实现像素级透明的前提。
     */
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);

    /* 设置高质量渲染选项 */
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

    /* 清空背景为全透明（Alpha=0） */
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

    /* 准备时间文本 */
    WCHAR timeStr[32];
    GetTimeString(timeStr, 32);

    /* 创建字体对象：使用配置中的字体名和大小 */
    Gdiplus::Font font(
        g_config.fontName,
        static_cast<Gdiplus::REAL>(g_config.fontSize),
        Gdiplus::FontStyleRegular,
        Gdiplus::UnitPixel
    );

    /* 创建画刷：颜色来自配置，Alpha通道控制整体透明度 */
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

    /* 绘制文本到内存Bitmap */
    Gdiplus::RectF layoutRect(
        0.0f, 0.0f,
        static_cast<Gdiplus::REAL>(width),
        static_cast<Gdiplus::REAL>(height)
    );
    graphics.DrawString(timeStr, -1, &font, layoutRect, &format, &brush);

    /*
     * 将绘制好的内存Bitmap输出到目标HDC。
     * 由于目标HDC关联的是32位DIB Section，Alpha信息会被保留，
     * 供后续的 UpdateLayeredWindow 使用。
     */
    Gdiplus::Graphics destGraphics(hdcDest);
    destGraphics.DrawImage(&bitmap, 0, 0);
}