#ifndef UTILS_H
#define UTILS_H

#include <windows.h>

HFONT CreateUiFont(int pointSize, LONG weight, const WCHAR* faceName);

void ApplyWindowRoundedCorners(HWND hWnd, int radius);
void CenterWindowOnParent(HWND hWnd, HWND hParent, int width, int height);

BOOL TryParseHexColorString(const WCHAR* hexStr, COLORREF* outColor);

#endif /* UTILS_H */