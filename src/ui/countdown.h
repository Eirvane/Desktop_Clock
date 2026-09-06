#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include <windows.h>

/* 显示倒计时时间设置窗口（滚动式列表选择时/分/秒） */
void CountdownSettings_Show(HWND hParent);

/* 【新增】显示正计时开始窗口（含开始按钮） */
void StopwatchStart_Show(HWND hParent);

#endif /* COUNTDOWN_H */