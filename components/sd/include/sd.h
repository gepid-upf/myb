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

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#ifndef SD_H
#define SD_H

// To enable SPI mode, uncomment the following line:
#define USE_SPI_MODE

/**
 * Pin mapping when using SPI mode.
 * With this mapping, SD card can be used both in SPI and 1-line SD mode.
 * Note that a pull-up on CS line is required in SD mode.
 */
#ifdef USE_SPI_MODE
#define PIN_NUM_MISO 17
#define PIN_NUM_MOSI 16
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 13
#endif

esp_err_t sd_init();
void sd_disable();
const char* sd_get_tag();
#endif
