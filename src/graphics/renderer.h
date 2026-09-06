#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* 初始化GDI+子系统，成功返回TRUE */
	BOOL Renderer_Init(void);

	/* 关闭GDI+，释放资源 */
	void Renderer_Shutdown(void);

	/* 在指定的HDC上绘制时钟（HDC须关联32位DIB Section以支持Alpha） */
	void Renderer_DrawClock(HDC hdcDest, int width, int height);

	/* 设置显示模式：0=当前时间, 1=正计时, 2=倒计时 */
	void Renderer_SetMode(int mode);

	/* 启动正计时（点击开始按钮时调用） */
	void Renderer_StartStopwatch(void);

	/* 启动倒计时（从当前时刻开始倒数指定时/分/秒） */
	void Renderer_StartCountdown(int hours, int minutes, int seconds);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_H */