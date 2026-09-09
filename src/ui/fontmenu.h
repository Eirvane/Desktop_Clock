#ifndef FONTMENU_H
#define FONTMENU_H

#include <windows.h>

typedef struct {
    UINT  id;
    WCHAR fileName[MAX_PATH];
    WCHAR displayName[64];
} FontMenuItem;

#define ID_MENU_OPEN_FONT_FOLDER 3100

HMENU BuildFontSubmenu(void);
void HandleFontMenuCommand(UINT cmdId);
void OpenFontsFolder(void);   

#endif /* FONTMENU_H */
