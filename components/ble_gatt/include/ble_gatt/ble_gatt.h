/**
 * @file ble_gatt.h
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * Gabriel Boni Vicari (133192@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 *
 * @copyright 2018 Angelo Elias Dalzotto, Gabriel Boni Vicari
 *
 * @brief GATT server for ESP32 ESP-IDF.
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#define TEST_DEVICE_NAME                "MYB_GATT"
#define GATTS_SERVICE_UUID_MYB          0xAABB
#define GATTS_CHAR_UUID_STEPCOUNT       0xAA01
#define GATTS_CHAR_UUID_BPM             0xBB01
#define GATTS_NUM_HANDLES               8

#define GATTS_DEMO_CHAR_VAL_LEN_MAX     0xFF    // Maximum value of a characteristic.
#define PROFILE_ON_APP_ID               0       // Profile info.
// Characteristic IDs for step count and BPM notification:
#define CHAR_NUM                        2
#define CHARACTERISTIC_STEPCOUNT_ID     0
#define CHARACTERISTIC_BPM_ID           1

// Value range of a attribute (characteristic):
uint8_t attr_str[] = {0x00};
esp_attr_value_t gatts_attr_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(attr_str),
    .attr_value = attr_str,
};

// Custom UUID:
static uint8_t service_uuid128[32] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t adv_params;

// Holds the information of characteristic:
struct gatts_characteristic_inst {
    esp_bt_uuid_t char_uuid;
    esp_bt_uuid_t descr_uuid;
    uint16_t char_handle;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
};

// Structure holds the information of current BLE connection:
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    struct gatts_characteristic_inst chars[CHAR_NUM];
};

//  This variable holds the information of current BLE connection:
static struct gatts_profile_inst profile;

/**
 * @brief This callback will be invoked when GAP advertising events come.
 *
 * @param event
 * @param *param
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

/**
 * @brief Function to be invoked to handle incomming events.
 *
 * @param event
 * @param gatts_if
 * @param *param
 */
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/**
 * @brief
 * 
 * @param event
 * @param gatts_if
 * @param *param
 */
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
#endif