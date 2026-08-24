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

/* 使用相对路径引用其他模块（若已配置 VS 附加包含目录，可去掉 ../ 前缀） */
#include "../platform/window.h"
#include "../graphics/renderer.h"
#include "../utils/config.h"
#include "../platform/trayicon.h"

/* 自动链接所需的系统库 */
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

    /* 设置高DPI感知（Per Monitor V2） */
    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContextProc pSetContext =
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetContext) {
            pSetContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    /* 初始化COM公寓线程模型（GDI+需要） */
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"COM init failed.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* 初始化GDI+ */
    if (!Renderer_Init()) {
        MessageBoxW(NULL, L"GDI+ init failed.", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    /* 加载配置（若首次运行则使用默认值并创建INI文件） */
    Config_Load();

    /* 注册窗口类 */
    if (!RegisterClockWindowClass(hInstance)) {
        MessageBoxW(NULL, L"Window class registration failed.", L"Error", MB_OK | MB_ICONERROR);
        Renderer_Shutdown();
        CoUninitialize();
        return 1;
    }

    /* 创建主窗口 */
    HWND hWnd = CreateClockWindow(hInstance);
    if (!hWnd) {
        MessageBoxW(NULL, L"CreateWindow failed.", L"Error", MB_OK | MB_ICONERROR);
        Renderer_Shutdown();
        CoUninitialize();
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    /* 创建系统托盘图标 */
    TrayIcon_Init(hWnd, hInstance);

    /* 设置定时器：每秒触发一次 WM_TIMER */
    SetTimer(hWnd, TIMER_ID_UPDATE, 1000, NULL);

    /* 标准Win32消息循环 */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* 清理资源 */
    KillTimer(hWnd, TIMER_ID_UPDATE);
    Renderer_Shutdown();
    CoUninitialize();

    return (int)msg.wParam;
}