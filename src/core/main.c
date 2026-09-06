#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include "../platform/window.h"
#include "../graphics/renderer.h"
#include "../utils/config.h"
#include "../platform/trayicon.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContextProc pSetContext =
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetContext) {
            pSetContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"COM init failed.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!Renderer_Init()) {
        MessageBoxW(NULL, L"GDI+ init failed.", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    Config_Load();

    /* 【新增】恢复上次保存的模式 */
    Renderer_SetMode(g_config.mode);
    if (g_config.mode == 2) {
        Renderer_StartCountdown(
            g_config.countdownHours,
            g_config.countdownMinutes,
            g_config.countdownSeconds);
    }

    if (!RegisterClockWindowClass(hInstance)) {
        MessageBoxW(NULL, L"Window class registration failed.", L"Error", MB_OK | MB_ICONERROR);
        Renderer_Shutdown();
        CoUninitialize();
        return 1;
    }

    HWND hWnd = CreateClockWindow(hInstance);
    if (!hWnd) {
        MessageBoxW(NULL, L"CreateWindow failed.", L"Error", MB_OK | MB_ICONERROR);
        Renderer_Shutdown();
        CoUninitialize();
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    TrayIcon_Init(hWnd, hInstance);

    SetTimer(hWnd, TIMER_ID_UPDATE, 1000, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(hWnd, TIMER_ID_UPDATE);
    Renderer_Shutdown();
    CoUninitialize();

    return (int)msg.wParam;
}