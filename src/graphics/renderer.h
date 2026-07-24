#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>

/* 使用C++编译器编译此头文件时，保持C链接约定 */
#ifdef __cplusplus
extern "C" {
#endif

	/* 初始化GDI+子系统，成功返回TRUE */
	BOOL Renderer_Init(void);

	/* 关闭GDI+，释放资源 */
	void Renderer_Shutdown(void);

	/* 在指定的HDC上绘制时钟（HDC须关联32位DIB Section以支持Alpha） */
	void Renderer_DrawClock(HDC hdcDest, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_H */