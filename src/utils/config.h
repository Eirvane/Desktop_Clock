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
        int     hourFormat;
        BOOL    topMost;
        BOOL    movable;
        WCHAR   fontName[64];
        WCHAR   fontFile[MAX_PATH];

        BOOL     showFrame;
        int      framePadding;
        int      frameBorderWidth;
        COLORREF frameColor;
        COLORREF frameFillColor;
        BYTE     frameAlpha;

        int      mode;               /* 0=当前时间, 1=正计时, 2=倒计时 */
        int      countdownHours;
        int      countdownMinutes;
        int      countdownSeconds;
    } AppConfig;

    extern AppConfig g_config;

    void Config_SetDefaults(void);
    void Config_Load(void);
    void Config_Save(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
