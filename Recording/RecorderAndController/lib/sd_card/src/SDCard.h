#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/sdmmc_types.h>
#include <driver/sdspi_host.h>

#include <string>
#define ONE_BIT_MODE true

class SDCard
{
private:
    std::string m_mount_point;
    //sdmmc_card_t *m_card;
    //sdmmc_host_t m_host = SDSPI_HOST_DEFAULT();

public:
    SDCard(const char *mount_point);
    ~SDCard();
    const std::string &get_mount_point() { return m_mount_point; }
    int getFileCount();
};
