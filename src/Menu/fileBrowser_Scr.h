
#ifndef FILE_BROWSER_SCR_H
#define FILE_BROWSER_SCR_H

#include "lcdUI.h"
#include "dirent.h"

class FileBrowser_Scr final: public Screen
{
public:
    FileBrowser_Scr(lcdUI* UI);

    void update(const uint32_t deltaTime) override;

    void render(tftLCD *tft) override;

    void handleTouch(const Screen::touchEvent event, const Vector2<int16_t> pos) override;

private:
    void printDirectory(File dir, int numTabs);
    void loadPage();
    bool isHidden(const char * name);

    const static uint32_t maxFilenameLen = 26;
    char path[64] = "/sdcard/";
    char dirList[8][maxFilenameLen];
    uint8_t isDir = 0;
    uint8_t numFilePages = 0;
    uint8_t filePage = 0;
    uint8_t pageLoaded = 1;
    uint8_t hiddenFiles[32] = {0};

    bool draw = false;
    bool init = false;
    Vector2<int16_t> cursor;
};

#endif