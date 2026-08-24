#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

#define TIMER_ID_UPDATE  1

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _AppConfig {
        int     x;
        int     y;
        int     width;
        int     height;
        int     fontSize;
        COLORREF textColor;
        BYTE    alpha;
        BOOL    showSeconds;
        BOOL    topMost;
        WCHAR   fontName[64];

        /* 【新增】字体文件路径，供 renderer 用 PrivateFontCollection 加载 */
        WCHAR   fontFile[MAX_PATH];

        /* 方框配置 */
        BOOL     showFrame;
        int      framePadding;
        int      frameBorderWidth;
        COLORREF frameColor;
        COLORREF frameFillColor;
        BYTE     frameAlpha;
    } AppConfig;

    extern AppConfig g_config;

    void Config_SetDefaults(void);
    void Config_Load(void);
    void Config_Save(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */