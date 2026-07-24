#include "config.h"
#include <stdio.h>

/* 全局配置实例 */
AppConfig g_config = { 0 };

/* 获取配置文件的完整路径（与exe同级目录） */
static void GetConfigFilePath(WCHAR* outPath, int maxLen)
{
    WCHAR exePath[MAX_PATH] = { 0 };

    /* 获取当前exe的完整路径 */
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    /* 找到最后一个反斜杠，截断路径，保留目录部分 */
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
    }

    /* 拼接 ini 文件名 */
    _snwprintf_s(outPath, maxLen, _TRUNCATE, L"%sclock.ini", exePath);
}

/* 设置默认配置值 */
void Config_SetDefaults(void)
{
    g_config.x = 100;
    g_config.y = 100;
    g_config.width = 400;
    g_config.height = 120;
    g_config.fontSize = 56;
    g_config.textColor = RGB(255, 255, 255);  /* 纯白 */
    g_config.alpha = 240;                       /* 约94%不透明 */
    g_config.showSeconds = TRUE;
    g_config.topMost = TRUE;
    wcscpy_s(g_config.fontName, 64, L"Microsoft YaHei UI");
}

/* 将十六进制颜色字符串解析为 COLORREF */
static COLORREF ParseHexColor(const WCHAR* hexStr)
{
    unsigned int r = 0, g = 0, b = 0;
    swscanf_s(hexStr, L"%2x%2x%2x", &r, &g, &b);
    return RGB(r, g, b);
}

/* 将 COLORREF 转换为十六进制字符串 */
static void ColorToHex(COLORREF color, WCHAR* out, size_t outSize)
{
    BYTE r = GetRValue(color);
    BYTE g = GetGValue(color);
    BYTE b = GetBValue(color);
    _snwprintf_s(out, outSize, _TRUNCATE, L"%02X%02X%02X", r, g, b);
}

void Config_Load(void)
{
    WCHAR buf[256];
    WCHAR configPath[MAX_PATH];

    GetConfigFilePath(configPath, MAX_PATH);
    Config_SetDefaults();

    g_config.x = GetPrivateProfileIntW(L"Window", L"X", g_config.x, configPath);
    g_config.y = GetPrivateProfileIntW(L"Window", L"Y", g_config.y, configPath);
    g_config.width = GetPrivateProfileIntW(L"Window", L"Width", g_config.width, configPath);
    g_config.height = GetPrivateProfileIntW(L"Window", L"Height", g_config.height, configPath);

    g_config.fontSize = GetPrivateProfileIntW(L"Appearance", L"FontSize", g_config.fontSize, configPath);
    g_config.alpha = (BYTE)GetPrivateProfileIntW(L"Appearance", L"Alpha", g_config.alpha, configPath);
    g_config.showSeconds = GetPrivateProfileIntW(L"Appearance", L"ShowSeconds", g_config.showSeconds, configPath);

    GetPrivateProfileStringW(L"Appearance", L"TextColor", L"FFFFFF", buf, 256, configPath);
    g_config.textColor = ParseHexColor(buf);

    GetPrivateProfileStringW(L"Appearance", L"FontName", g_config.fontName, g_config.fontName, 64, configPath);
}

void Config_Save(void)
{
    WCHAR buf[256];
    WCHAR configPath[MAX_PATH];

    GetConfigFilePath(configPath, MAX_PATH);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.x);
    WritePrivateProfileStringW(L"Window", L"X", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.y);
    WritePrivateProfileStringW(L"Window", L"Y", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.width);
    WritePrivateProfileStringW(L"Window", L"Width", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.height);
    WritePrivateProfileStringW(L"Window", L"Height", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.fontSize);
    WritePrivateProfileStringW(L"Appearance", L"FontSize", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.alpha);
    WritePrivateProfileStringW(L"Appearance", L"Alpha", buf, configPath);

    _snwprintf_s(buf, 256, _TRUNCATE, L"%d", g_config.showSeconds);
    WritePrivateProfileStringW(L"Appearance", L"ShowSeconds", buf, configPath);

    ColorToHex(g_config.textColor, buf, 256);
    WritePrivateProfileStringW(L"Appearance", L"TextColor", buf, configPath);

    WritePrivateProfileStringW(L"Appearance", L"FontName", g_config.fontName, configPath);
}