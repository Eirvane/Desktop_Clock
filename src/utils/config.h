#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

/* 定时器ID：用于每秒刷新时钟 */
#define TIMER_ID_UPDATE  1

#ifdef __cplusplus
extern "C" {
#endif

    /* 应用程序配置结构体 */
    typedef struct _AppConfig {
        int     x;              /* 窗口X坐标 */
        int     y;              /* 窗口Y坐标 */
        int     width;          /* 窗口宽度 */
        int     height;         /* 窗口高度 */
        int     fontSize;       /* 字体大小（像素） */
        COLORREF textColor;     /* 文本颜色（RGB） */
        BYTE    alpha;          /* 整体透明度（0-255，255为不透明） */
        BOOL    showSeconds;    /* 是否显示秒数 */
        BOOL    topMost;        /* 是否置顶 */
        WCHAR   fontName[64];   /* 字体名称 */
    } AppConfig;

    /* 全局配置实例，供各模块直接读取 */
    extern AppConfig g_config;

    /* 配置操作函数 */
    void Config_SetDefaults(void);
    void Config_Load(void);
    void Config_Save(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */