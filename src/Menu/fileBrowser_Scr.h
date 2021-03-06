
#ifndef FILE_BROWSER_SCR_H
#define FILE_BROWSER_SCR_H

#include "lcdUI.h"
#include "dirent.h"

class FileBrowser_Scr final: public Screen
{
public:
    FileBrowser_Scr(lcdUI* UI, tftLCD& tft);

    void update(const uint32_t deltaTime) override;

    void render(tftLCD& tft) override;

    void handleTouch(const Screen::touchEvent event, const Vec2h pos) override;

private:
    void printDirectory(File dir, const int numTabs);
    void loadPage();
    void renderPage(tftLCD& tft);
    bool isPageLoaded();
    void setPageLoaded();
    void updatePath(const std::string &newPath, const bool relativePath);
    bool isHidden(const char * name);
    bool isGcode(const std::string &file);
    void sendFile(const std::string &file);

    std::string path = "/sdcard";
    std::string dirList[8];
    uint8_t fileDepth = 0;
    uint8_t isDir = 0;
    uint8_t numFilePages = 0;
    uint8_t filePage = 0;
    uint8_t pageLoaded = 1;
    bool pageRendered = false;
    uint8_t hiddenFiles[32] = {0};
};

#endif