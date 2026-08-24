#ifndef FONTMENU_H
#define FONTMENU_H

#include <windows.h>

/* 字体菜单项结构 */
typedef struct {
    UINT  id;                      /* 菜单命令 ID */
    WCHAR fileName[MAX_PATH];      /* 字体文件名（含扩展名） */
    WCHAR displayName[64];         /* 菜单显示名（去扩展名，下划线转空格） */
} FontMenuItem;

/* 扫描 fonts 目录并构建弹出子菜单；返回 HMENU 所有权移交调用方 */
HMENU BuildFontSubmenu(void);

/* 处理字体菜单项点击：加载字体并应用到配置 */
void HandleFontMenuCommand(UINT cmdId);

#endif /* FONTMENU_H */