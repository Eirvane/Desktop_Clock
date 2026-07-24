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

    /* 绘制半透明方框（如果启用） */
    if (g_config.showFrame) {
        int pad = g_config.framePadding;
        int bw = g_config.frameBorderWidth;

        /* 确保方框区域有效（内边距不能太大） */
        int frameW = width - pad * 2;
        int frameH = height - pad * 2;
        if (frameW > 0 && frameH > 0) {
            Gdiplus::RectF frameRect(
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)pad,
                (Gdiplus::REAL)frameW,
                (Gdiplus::REAL)frameH
            );

            /* 填充半透明背景 */
            Gdiplus::SolidBrush fillBrush(
                Gdiplus::Color(
                    g_config.frameAlpha,
                    GetRValue(g_config.frameFillColor),
                    GetGValue(g_config.frameFillColor),
                    GetBValue(g_config.frameFillColor)
                )
            );
            graphics.FillRectangle(&fillBrush, frameRect);

            /* 绘制边框 */
            if (bw > 0) {
                Gdiplus::Pen framePen(
                    Gdiplus::Color(
                        g_config.alpha,  /* 边框使用与文字相同的透明度 */
                        GetRValue(g_config.frameColor),
                        GetGValue(g_config.frameColor),
                        GetBValue(g_config.frameColor)
                    ),
                    (Gdiplus::REAL)bw
                );
                /* 调整矩形使边框居中绘制（Pen的宽度会向两侧扩展） */
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