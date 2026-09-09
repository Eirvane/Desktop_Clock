#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	BOOL Renderer_Init(void);
	void Renderer_Shutdown(void);
	void Renderer_DrawClock(HDC hdcDest, int width, int height);
	void Renderer_SetMode(int mode);
	void Renderer_StartStopwatch(void);
	void Renderer_StartCountdown(int hours, int minutes, int seconds);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_H */