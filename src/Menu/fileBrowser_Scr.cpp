
#include "fileBrowser_Scr.h"

FileBrowser_Scr::FileBrowser_Scr(lcdUI* UI, tftLCD& tft):
    Screen(UI)
{
    tft.fillScreen(TFT_BLACK);
    /* tft.drawRect(0, 0, 480, 70, TFT_RED);
    
    tft.setTextDatum(CC_DATUM);
    tft.setTextFont(4);
    tft.drawString("SD card", 240, 35);
    tft.setTextFont(2); */

    // SD Home
    tft.drawRoundRect(0, 0, 50, 50, 4, TFT_ORANGE);
    tft.drawBmpSPIFFS("/spiffs/home_24.bmp", 13, 13);

    tft.drawBmpSPIFFS("/spiffs/return_48.bmp", 53, 272);
    tft.drawRoundRect(0, 256, 155, 64, 4, TFT_ORANGE);
    tft.drawRoundRect(288, 256, 66, 64, 4, TFT_CYAN);
}

void FileBrowser_Scr::update(const uint32_t deltaTime)
{
    if (_UI->checkSD())
    {
        /* if (!init)
        {
            printDirectory(SD.open("/"), 0);
            printf("SPIFFS:\n");
            printDirectory(SPIFFS.open("/"), 0);

            init = true;
        } */

        loadPage();
    }
}

void FileBrowser_Scr::render(tftLCD& tft)
{
    if (_UI->checkSD())
    {
        renderPage(tft);
    }
    else
    {
        tft.setTextDatum(CC_DATUM);
        tft.drawString("SD not found :(", 240, 195);
    }
}

bool FileBrowser_Scr::isPageLoaded()
{
    return pageLoaded == filePage;
}

void FileBrowser_Scr::setPageLoaded()
{
    pageLoaded = filePage;
    pageRendered = false;
}

void FileBrowser_Scr::updatePath(const std::string &newPath, const bool relativePath)
{
    if (relativePath)
    {
        size_t dot = newPath.find("/..");
        if (dot == -1)
        {
            path += "/" + newPath;
        } 
        else
        {
            dot += 3;
            size_t slash = path.rfind('/');
            if (slash == -1 || (newPath[dot] < '0' && newPath[dot] > '9'))
            {
                ESP_LOGE("fileBrowser", "Invalid path!");
                return;
            }

            for (uint8_t i = newPath[dot]-'0'; i > 0; i--)
            {
                if (path.compare("/sdcard") == 0) break;
                path.erase(slash);
                slash = path.rfind('/');
                if (slash == -1) return;
            }
        }
    } else
    {
        path = newPath;
    }
    ESP_LOGD("fileBrowser", "New path: %s", path.c_str());
    numFilePages = 0;       // Mark as new folder for reading
    filePage++;             // Trigger page load
}

void FileBrowser_Scr::handleTouch(const touchEvent event, const Vec2h pos)
{
    if (event == press)
    {
        if (pos.y < 50) // Navigation bar
        {
            if (pos.x < 50) updatePath("/sdcard", false);       // Return Home
            uint8_t k = 1;
            for (uint8_t i = fileDepth > 5? 5 : fileDepth; i > 0; i--)
            {
                if (pos.x >= 86*i-36 && pos.x < 50 + 86*i)
                {
                    switch (k)
                    {
                    case 1:
                        filePage = 0;
                        break;

                    case 2:
                        updatePath("/..1", true);
                        break;

                    case 3:
                        updatePath("/..2", true);
                        break;

                    case 4:
                        updatePath("/..3", true);
                        break;

                    case 5:
                        updatePath("/..4", true);
                        break;
                    }
                }
                k++;
            }
        }
        else if (pos.y < 250) // Folder content
        {
            uint8_t idx = (pos.y - 50) / 50;
            if (pos.x >= 240) idx += 4;
            ESP_LOGV("Touch", "IDX: %d", idx);
            if ((isDir & (1 << idx)) > 0) updatePath(dirList[idx], true);
            else if (isGcode(dirList[idx]))
                sendFile(dirList[idx]);
        }
        else // Menu buttons
        {
            if (pos.x < 160)
            {
                _UI->setScreen(lcdUI::Info);
            }
            else if (pos.x < 280 && filePage > 0) filePage--;
            else if (pos.x >= 362 && filePage < numFilePages-1) filePage++;
        }
    }
}

void FileBrowser_Scr::printDirectory(File dir, int numTabs)
{
    while (true)
    {
        File entry =  dir.openNextFile();

        if (!entry) break; // no more files

        for (uint8_t i = 0; i < numTabs; i++)
        {
            Serial.print('\t');
        }

        // strrchar() gets the last occurence of '/' get only the name of the file
        Serial.print(strrchr(entry.name(), '/'));

        if (entry.isDirectory())
        {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        }
        else
        {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}

void FileBrowser_Scr::renderPage(tftLCD& tft)
{
    if (pageRendered) return;

    // Draw controls
    // Page +
    if (pageLoaded == 0)
    {
        tft.fillRect(163, 256, 117, 64, TFT_BLACK);
    }
    else
    {
        tft.drawBmpSPIFFS("/spiffs/arrowL_48.bmp", 197, 268);
        tft.drawRoundRect(163, 256, 117, 64, 4, TFT_ORANGE);
    }

    // Page -
    if (pageLoaded == numFilePages-1)
    {
        tft.fillRect(362, 256, 118, 64, TFT_BLACK);
    }
    else
    {
        tft.drawBmpSPIFFS("/spiffs/arrowR_48.bmp", 397, 268);
        tft.drawRoundRect(362, 256, 118, 64, 4, TFT_ORANGE);
    }
    
    // Folder path names
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextPadding(0);
    tft.setTextDatum(CC_DATUM);

    uint8_t idx = path.length()-1;
    for (uint8_t i = 5; i > 0; i--) // For each of the last 5 folders
    {
        if (i <= fileDepth)
        {
            tft.fillRect(86*i - 35, 1, 84, 48, TFT_BLACK);
            tft.drawRoundRect(86*i - 36, 0, 86, 50, 4, TFT_ORANGE);
            uint8_t len = 0;
            for (; idx > 0; idx--, len++)
            {
                if (path[idx] == '/') break;
            }

            std::string folderName = path.substr(idx-- +1, len);
            tft.drawStringWr(folderName.c_str(), 7 + 86*i, 25, 80, 48);
        }
        else
        {
            tft.fillRect(86*i - 36, 0, 86, 50, TFT_BLACK);
        }
    }
    
    tft.setTextPadding(48);
    tft.drawString("Page", 321, 280);
    tft.drawString(String(pageLoaded+1) + " of " + String(numFilePages), 321, 296);

    // Draw file table
    tft.setTextDatum(CL_DATUM);
    tft.setTextPadding(202);                                   // Set pading to clear old text
    uint8_t k = 0;
    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t j = 0; j < 4; j++, k++)
        {
            if (dirList[k].length() == 0)
            {
                tft.fillRect(241*i, 51 + 50*j, 239, 48, TFT_BLACK);   // Clear unused slots
                continue;
            }
            tft.fillRect(10 + 240*i, 52 + 50*j, 200, 46, TFT_BLACK);
     
            tft.drawStringWr(dirList[k].c_str(), 10 + 240*i, 75 + 50*j, 200, 48);       // Write file name

            if ((isDir & 1<<k) > 0)
                tft.drawBmpSPIFFS("/spiffs/folder_24.bmp", 212 + 240*i, 63 + 50*j);     // Draw folder icon
            else if (isGcode(dirList[k]))
                tft.drawBmpSPIFFS("/spiffs/gcode_24.bmp", 212 + 240*i, 63 + 50*j);      // Draw gcode icon
            else
                tft.drawBmpSPIFFS("/spiffs/file_24.bmp", 212 + 240*i, 63 + 50*j);       // Draw file icon
            
            tft.drawRoundRect(241*i, 51 + 50*j, 239, 48, 4, TFT_OLIVE);       // Draw grid
        }
    }
    pageRendered = true;
}

void FileBrowser_Scr::loadPage()
{
    if (isPageLoaded()) return;
    struct dirent *entry;
    DIR * dir = opendir(path.c_str());
    if (!dir)
    {
        ESP_LOGE("fileBrowser", "Error opening folder %s", path);
        return;
    }

    if (numFilePages == 0) // First directory read
    {
        uint16_t hidden = 0;
        while ((entry = readdir(dir)) && numFilePages < 255)
        {
            if (isHidden(entry->d_name))
            {
                hidden++;
                continue;
            }
            numFilePages++;      // Count number of files

            if (numFilePages % 8 == 0) hiddenFiles[numFilePages / 8] = hidden;
        }
        numFilePages /= 8;                                  // Translate to screen pages
        numFilePages++;
        ESP_LOGD("fileBrowser", "File Pages: %d", numFilePages);

        fileDepth = 0;
        for (uint8_t i = 1; i < path.length(); i++)
        {
            if (path[i] == '/') fileDepth++;
        }

        rewinddir(dir);
        filePage = 0;
    }

    isDir = 0;
    seekdir(dir, filePage*8 + hiddenFiles[filePage]);
    uint8_t i = 0;
    while (i < 8)
    {
        entry = readdir(dir);
        if (!entry) break;                                  // No more files

        if (isHidden(entry->d_name)) continue;
        
        dirList[i] = entry->d_name;                         // Save file name
        
        if (entry->d_type == DT_DIR) isDir |= 1<<i;
        i++;
    }

    closedir(dir);

    for ( ; i < 8; i++)
    {
        dirList[i].clear();
    }
    setPageLoaded();
}

bool FileBrowser_Scr::isHidden(const char *name)
{
    //const char *lookupTable[1] = {"System Volume Information"};
    if (strncmp(name, "System Volume Information", 256) == 0) return true;

    // May add extra system files here

    return false;
}

bool FileBrowser_Scr::isGcode(const std::string &file)
{
    size_t dot = file.rfind('.');
    if (dot != -1)
    {
        dot++;
        if (file.compare(dot, 5, "g") == 0 || file.compare(dot, 5, "gco") == 0 || file.compare(dot, 5, "gcode") == 0)
            return true;
    }
    return false;
}

void FileBrowser_Scr::sendFile(const std::string &file)
{
    if (_UI->setFile(path + "/" + file))
        _UI->setScreen(lcdUI::GcodePreview);
}
