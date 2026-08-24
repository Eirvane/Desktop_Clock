#include "fontmenu.h"
#include "window.h"   /* g_hClockWnd, UpdateLayeredWindowContent, ID_MENU_FONT_BASE */
#include "config.h"
#include <wchar.h>

#pragma comment(lib, "gdi32.lib")   /* AddFontResourceExW */

/* 扫描结果缓存：避免每次弹出菜单都重复读取磁盘 */
static FontMenuItem g_fontMenuItems[MAX_FONT_MENU_ITEMS];
static int g_fontMenuItemCount = 0;

/* ------------------------------------------------------------------ */
/* 运行时动态获取字体目录（与 exe 同级目录下的 fonts 文件夹）           */
/* ------------------------------------------------------------------ */

static void GetFontsFolderPath(WCHAR* outPath, int maxLen)
{
    WCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
    }

    _snwprintf_s(outPath, maxLen, _TRUNCATE, L"%sfonts", exePath);
}

/* 检查扩展名是否为支持的字体格式 */
static BOOL IsFontFileExtension(const WCHAR* ext)
{
    if (!ext) return FALSE;
    return (_wcsicmp(ext, L".ttf") == 0 ||
        _wcsicmp(ext, L".otf") == 0 ||
        _wcsicmp(ext, L".ttc") == 0 ||
        _wcsicmp(ext, L".fon") == 0);
}

/* 从路径提取文件名（去扩展名），并将下划线替换为空格 */
static void ExtractFontDisplayName(const WCHAR* filePath, WCHAR* outName, int outSize)
{
    const WCHAR* nameStart = wcsrchr(filePath, L'\\');
    if (!nameStart) nameStart = filePath;
    else nameStart++;

    const WCHAR* ext = wcsrchr(nameStart, L'.');
    size_t len = ext ? (size_t)(ext - nameStart) : wcslen(nameStart);
    if (len >= (size_t)outSize) len = (size_t)outSize - 1;

    wcsncpy_s(outName, outSize, nameStart, len);
    outName[len] = L'\0';

    for (int i = 0; outName[i]; i++) {
        if (outName[i] == L'_') outName[i] = L' ';
    }
}

/* 扫描字体目录，将结果缓存到全局数组 */
static void ScanFontsFolder(void)
{
    g_fontMenuItemCount = 0;
    ZeroMemory(g_fontMenuItems, sizeof(g_fontMenuItems));

    WCHAR fontFolder[MAX_PATH] = { 0 };
    GetFontsFolderPath(fontFolder, MAX_PATH);

    WCHAR searchFilter[MAX_PATH] = { 0 };
    _snwprintf_s(searchFilter, MAX_PATH, _TRUNCATE, L"%s\\*", fontFolder);

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchFilter, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        const WCHAR* ext = wcsrchr(findData.cFileName, L'.');
        if (!IsFontFileExtension(ext)) continue;

        if (g_fontMenuItemCount >= MAX_FONT_MENU_ITEMS) break;

        FontMenuItem* item = &g_fontMenuItems[g_fontMenuItemCount];
        item->id = ID_MENU_FONT_BASE + g_fontMenuItemCount;
        wcsncpy_s(item->fileName, MAX_PATH, findData.cFileName, _TRUNCATE);
        ExtractFontDisplayName(findData.cFileName, item->displayName, 64);

        g_fontMenuItemCount++;
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

/* 构建字体子菜单（动态扫描目录，当前选中项打勾） */
HMENU BuildFontSubmenu(void)
{
    HMENU hFontMenu = CreatePopupMenu();
    if (!hFontMenu) return NULL;

    ScanFontsFolder();

    if (g_fontMenuItemCount == 0) {
        AppendMenuW(hFontMenu, MF_STRING | MF_GRAYED, 0, L"(无字体文件)");
    }
    else {
        for (int i = 0; i < g_fontMenuItemCount; i++) {
            UINT flags = MF_STRING;
            if (_wcsicmp(g_config.fontName, g_fontMenuItems[i].displayName) == 0) {
                flags |= MF_CHECKED;
            }
            AppendMenuW(hFontMenu, flags, g_fontMenuItems[i].id, g_fontMenuItems[i].displayName);
        }
    }

    return hFontMenu;
}

/* 加载指定字体文件（FR_PRIVATE：仅当前进程可见，退出自动卸载）并应用 */
void HandleFontMenuCommand(UINT cmdId)
{
    int index = (int)(cmdId - ID_MENU_FONT_BASE);
    if (index < 0 || index >= g_fontMenuItemCount) return;

    WCHAR fontFolder[MAX_PATH] = { 0 };
    GetFontsFolderPath(fontFolder, MAX_PATH);

    WCHAR filePath[MAX_PATH];
    _snwprintf_s(filePath, MAX_PATH, _TRUNCATE,
        L"%s\\%s", fontFolder, g_fontMenuItems[index].fileName);

    /* 加载到当前进程，不写入系统字体库 */
    int added = AddFontResourceExW(filePath, FR_PRIVATE, 0);
    (void)added; /* 静默容错：GDI+ 可能已通过其他途径识别 */

    /* 更新配置并重绘 */
    wcsncpy_s(g_config.fontName, 64, g_fontMenuItems[index].displayName, _TRUNCATE);
    Config_Save();
    UpdateLayeredWindowContent(g_hClockWnd);
}