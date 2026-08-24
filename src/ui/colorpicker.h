#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <windows.h>

/* 显示 HEX 颜色值输入对话框 */
void ColorValueInput_Show(HWND hParent);

/* 显示系统标准颜色选择对话框 */
void ColorPicker_Show(HWND hParent);

#endif /* COLORPICKER_H */