#include <freertos/FreeRTOS.h>
#include <sys/unistd.h>
#include "SDCard.h"
#include <SD_MMC.h>
#include <FS.h>

static const char *TAG = "SDC";

SDCard::SDCard(const char *mount_point)
{
    m_mount_point = mount_point;
    //if(!SD_MMC.begin("/sdcard", ONE_BIT_MODE)){
    if(!SD_MMC.begin(m_mount_point.c_str(), ONE_BIT_MODE)){
        Serial.println("Card Mount Failed");
        return;
    }
    ESP_LOGI(TAG, "SDCard mounted at: %s", m_mount_point.c_str());

    // Card has been initialized, print its properties
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    ESP_LOGI(TAG, "SD_MMC Card Size: %lluMB\n", cardSize);
}

SDCard::~SDCard()
{
    // All done, unmount partition and disable SDMMC or SPI peripheral
    SD_MMC.end();
    ESP_LOGI(TAG, "Card unmounted");
}

int SDCard::getFileCount() {
    fs::FS &fs = SD_MMC;
    File root = fs.open("/");

    // only consider files in thi root directory
    int fileCount = 0;
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            return fileCount;
        }
        fileCount++;
        entry.close();
    }
}