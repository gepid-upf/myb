/**
 * @file sd.c
 * 
 * @author
 * Gabriel Boni Vicari (133192@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 *
 * @copyright 2018 Gabriel Boni Vicari
 *
 * @brief SD library for ESP32 ESP-IDF. 
 */

#include "sd.h"

const char *TAG_SDMMC = "SDMMC";

/**
 * @brief Configure and initialize SD.
 * When testing SD and SPI modes, keep in mind that once the card has been
 * initialized in SPI mode, it can not be reinitialized in SD mode without
 * toggling power to the card.
 * 
 * @return Status of the SD module.
 */
esp_err_t sd_init()
{
    /**
     * This initializes the slot without card detect (CD) and write protect (WP)
     * signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
     * has these signals.   
     */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;

    // Options for mounting the filesystem:
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Use settings defined to initialize SD card and mount FAT filesystem:
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    return (ret);
}

/**
 * @brief Unmount partition and disable SDMMC or SPI peripheral.
 */
void sd_disable()
{
    esp_vfs_fat_sdmmc_unmount();
}

/**
 * @brief Get SDMMC Tag.
 * 
 * @return SDMMC tag.
 */
const char* sd_get_tag()
{
    return (TAG_SDMMC);
}