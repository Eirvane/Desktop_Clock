#ifndef UTILS_H
#define UTILS_H

#include <windows.h>

/* 创建 UI 字体（点大小 -> 像素高度，支持 ClearType） */
HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName);

/* 应用 DWM 圆角 + HRGN 圆角（Win11 / Win10 20H1+），失败则静默回退 */
/* radius: 圆角半径（像素），传入 0 则仅尝试 DWM 圆角 */
void ApplyWindowRoundedCorners(HWND hWnd, int radius);

/* 将窗口居中于父窗口；若父窗口无效则居中于屏幕 */
void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height);

/* 解析 HEX 颜色字符串（如 "FF5733"）为 COLORREF */
BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor);

#endif /* UTILS_H */