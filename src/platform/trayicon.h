#ifndef TRAYICON_H
#define TRAYICON_H

#include <windows.h>

void TrayIcon_Init(HWND hWnd, HINSTANCE hInstance);
void TrayIcon_Remove(HWND hWnd);

#endif /* TRAYICON_H */