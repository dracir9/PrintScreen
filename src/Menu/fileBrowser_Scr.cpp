
#include "fileBrowser_Scr.h"

FileBrowser_Scr::FileBrowser_Scr(lcdUI* UI)
{
    UI->tft.fillScreen(TFT_BLACK);
    _UI = UI;
    _UI->tft.drawRect(0, 0, 480, 70, TFT_RED);
    
    _UI->tft.setTextDatum(CC_DATUM);
    _UI->tft.setTextFont(4);
    _UI->tft.drawString("SD card", 240, 35);
    _UI->tft.setTextFont(2);
}

void FileBrowser_Scr::update(const uint32_t deltaTime)
{
    if (_UI->checkSD())
    {
        if (!init)
        {
            printDirectory(SD.open("/"), 0);

            printf("SPIFFS:\n");
            printDirectory(SPIFFS.open("/"), 0);
            /* if (SD.exists("/test.gcode"))
            {
                strcpy(_UI->selectedFile, "/sd/test.gcode");
                _UI->setScreen(_UI->GcodePreview);
            } */
            init = true;
        }

        loadPage();
    }
}

void FileBrowser_Scr::render(tftLCD *tft)
{
    if (_UI->checkSD())
    {
        renderPage(tft);
    }
    else
    {
        tft->setTextDatum(CC_DATUM);
        tft->drawString("SD not found :(", 240, 195);
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

void FileBrowser_Scr::updatePath(const char* newPath, const bool relativePath)
{
    if (relativePath)
    {
        char* dot = strstr(newPath, "/..");
        if (dot == NULL)
        {
            strcat(path, "/");
            strcat(path, newPath);
        } else
        {
            dot += 3;
            char* slash = strrchr(path, '/');
            if (slash == NULL || (*dot < '0' && *dot > '9'))
            {
                ESP_LOGE("fileBrowser", "Invalid path!");
                return;
            }

            for (uint8_t i = 0; i < (*dot-'0'); i++)
            {
                if (strcmp(path, "/sdcard") == 0) break;
                slash = strrchr(path, '/');
                if (slash == NULL) return;
                *slash = '\0';
            }
        }
    } else
    {
        strcpy(path, newPath);
    }
    ESP_LOGD("fileBrowser", "New path: %s", path);
    numFilePages = 0;       // Mark as new folder for reading
    filePage++;             // Trigger page load
    pageRendered = false;         // Trigger page render
}

void FileBrowser_Scr::handleTouch(const touchEvent event, const Vector2<int16_t> pos)
{
    if (event == press)
    {
        if (pos.y >= 70 && pos.y < 120)
        {
            if (pos.x < 50) updatePath("/sdcard", false);       // Return Home
            for (uint8_t i = 0, k = fileDepth > 4? 4 : fileDepth; i < fileDepth && i < 4; i++, k--)
            {
                if (pos.x >= 50 + 70*i && pos.x < 120 + 70*i)
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
                    
                    default:
                        break;
                    }
                }
            }
            if (pos.x >= 330 && pos.x < 380 && filePage > 0) filePage--;
            else if (pos.x >= 430 && filePage < numFilePages-1) filePage++;
        }
        else if (pos.y >= 120)
        {
            uint8_t idx = (pos.y - 120) / 50;
            if (pos.x >= 240) idx += 4;
            ESP_LOGD("Touch", "IDX: %d", idx);
            if ((isDir & (1 << idx)) > 0) updatePath(dirList[idx], true);
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

void FileBrowser_Scr::renderPage(tftLCD *tft)
{
    if (pageRendered) return;

    // Draw controls
    // Page +
    if (pageLoaded == 0)
        tft->fillRect(330, 70, 50, 50, TFT_BLACK);
    else
    {
        tft->drawBmpSPIFFS("/spiffs/arrowL_32.bmp", 335, 82);
        tft->drawRect(330, 70, 42, 50, TFT_CYAN);
    }

    // Page -
    if (pageLoaded == numFilePages-1)
        tft->fillRect(430, 70, 50, 50, TFT_BLACK);
    else
    {
        tft->drawBmpSPIFFS("/spiffs/arrowR_32.bmp", 443, 82);
        tft->drawRect(438, 70, 42, 50, TFT_CYAN);
    }

    // SD Home
    tft->drawRect(0, 70, 50, 50, TFT_ORANGE);
    tft->drawBmpSPIFFS("/spiffs/home_24.bmp", 13, 83);

    // Folder path
    for (uint8_t i = 0; i < 4; i++)
    {
        if (i < fileDepth)
        {
            tft->drawRect(50 + 70*i, 70, 70, 50, TFT_ORANGE);
            tft->fillRect(51 + 70*i, 71, 68, 48, TFT_BLACK);
        }
        else
        {
            tft->fillRect(50 + 70*i, 70, 70, 50, TFT_BLACK);
        }
    }
    
    // Folder path names
    char tmp[17];
    uint8_t k;
    uint8_t idx = strlen(path)-1;
    tft->setTextFont(2);
    tft->setTextPadding(68);
    tft->setTextDatum(CC_DATUM);
    
    for (uint8_t i = fileDepth > 4? 4 : fileDepth; i > 0; i--)
    {
        uint8_t cnt = 0, len = 0;
        for (uint8_t j = idx; j > 0; j--)
        {
            if (path[idx--] == '/') break;
            len++;
        }

        char *p = (char*)calloc(len+1, sizeof(char));
        strncpy(p, &path[idx+2], len);
        p[len] = '\0';

        uint8_t lines = tft->textWidth(p)/68;
        
        for (k = 0; k < 4; k++)
        {
            uint8_t charN = 15;
            strncpy(tmp, &p[cnt], 16);
            tmp[16] = '\0';
            
            while (tft->textWidth(tmp) > 68)
            {
                tmp[charN--] = '\0';
            }
            
            char *p = strchr(tmp, '/');
            if (p) *p = '\0';

            tft->drawString(tmp, 15 + 70*i, 95 - 8*lines + 16*k);

            cnt += charN + 1;
            if (cnt > len) break; // All characters read
        }
        free(p);
    }
    
    tft->setTextPadding(48);
    tft->drawString("Page", 405, 87);
    sprintf(tmp, "%d of %d", pageLoaded+1, numFilePages);
    tft->drawString(tmp, 405, 103);

    // Draw file table
    tft->setTextDatum(CL_DATUM);
    k = 0;
    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if (strlen(dirList[k]) == 0)
            {
                tft->fillRect(240*i, 120 + 50*j, 240, 50, TFT_BLACK);   // Clear unused slots
                k++;
                continue;
            }
            tft->drawRect(240*i, 120 + 50*j, 240, 50, TFT_OLIVE);       // Draw grid
            tft->setTextPadding(202);                                   // Set pading to clear old text
            tft->drawString(dirList[k], 10 + 240*i, 145 + 50*j);        // Write file name
            if ((isDir & 1<<k) > 0)
            {
                tft->drawBmpSPIFFS("/spiffs/folder_24.bmp", 212 + 240*i, 133 + 50*j);   // Draw folder icon
            }
            else
            {
                tft->drawBmpSPIFFS("/spiffs/file_24.bmp", 212 + 240*i, 133 + 50*j);     // Draw file icon
            }
            k++;
        }
    }
    pageRendered = true;
}

void FileBrowser_Scr::loadPage()
{
    if (isPageLoaded()) return;
    struct dirent *entry;
    DIR * dir = opendir(path);
    if (!dir)
    {
        ESP_LOGE("fileBrowser", "Error opening folder %s", path);
        return;
    }

    if (numFilePages == 0) // First directory read
    {
        uint16_t hidden = 0;
        while ((entry = readdir(dir)) && numFilePages <= 255)
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
        for (char *p = strrchr(path, '/'); p > path; p--)
        {
            if (*p == '/') fileDepth++;
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
        
        strncpy(dirList[i], entry->d_name, maxFilenameLen-1); // Save file name and cut to size
        dirList[i][maxFilenameLen-1] = '\0';
        
        if (entry->d_type == DT_DIR) isDir |= 1<<i;
        i++;
    }

    closedir(dir);

    for ( ; i < 8; i++)
    {
        dirList[i][0] = '\0';
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
