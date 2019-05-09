#include "ble_gatt/ble_gatt.h"

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            printf("GATT: Advertising start failed.\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            printf("GATT: Advertising stop failed.\n");
        }
        else {
            printf("GATT: Stop advertising successfully.\n");
        }
        break;
    default:
        break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        // When register application ID, the event comes:
        case ESP_GATTS_REG_EVT: {
            printf("GATT: REGISTER_APP_EVT: status: %d, ID: %d.\n", param->reg.status, param->reg.app_id);
            profile.service_id.is_primary = true;
            profile.service_id.id.inst_id = 0x00;
            profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
            profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_MYB;
            /**
             * After finishing registering, the ESP_GATTS_REG_EVT event comes,
             * we start the next step is creating service:
             */
            esp_ble_gatts_create_service(gatts_if, &profile.service_id, GATTS_NUM_HANDLES);
            break;
        }
        case ESP_GATTS_READ_EVT: {
            printf("GATT: ESP_GATTS_READ_EVT.\n");
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 3;
            rsp.attr_value.value[0] = 109;
            rsp.attr_value.value[1] = 121;
            rsp.attr_value.value[2] = 98;

            /**
             * When central device send READ request to GATT server,
             * the ESP_GATTS_READ_EVT comes.
             * This always responds "myb" string.
             */
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;
        }
        /**
         * When central device send WRITE request to GATT server, the
         * ESP_GATTS_WRITE_EVT comes. 
         * Invoking process_write_event_env() to process and send response if
         * any.
         */
        case ESP_GATTS_WRITE_EVT: {
            printf("GATT: ESP_GATTS_WRITE_EVT.\n");
            process_write_event_env(gatts_if, param);
            break;
        }
        // When create service complete, the event comes.
        case ESP_GATTS_CREATE_EVT: {
            printf("GATT: status: %d, handle: %x, ID: %x.\n", param->create.status, param->create.service_handle, param->create.service_id.id.uuid.uuid.uuid16);
            // Store service handle and add characteristics.
            profile.service_handle = param->create.service_handle;
            // Step counter characteristic:
            esp_ble_gatts_add_char
            (
                profile.service_handle,
                &profile.chars[CHARACTERISTIC_STEPCOUNT_ID].char_uuid,
                profile.chars[CHARACTERISTIC_STEPCOUNT_ID].perm,
                profile.chars[CHARACTERISTIC_STEPCOUNT_ID].property,
                &gatts_attr_val, NULL
            );
            // BPM monitoring characteristic:
            esp_ble_gatts_add_char
            (
                profile.service_handle, 
                &profile.chars[CHARACTERISTIC_BPM_ID].char_uuid,
                profile.chars[CHARACTERISTIC_BPM_ID].perm,
                profile.chars[CHARACTERISTIC_BPM_ID].property,
                &gatts_attr_val, NULL
            );
            // And start service:
            esp_ble_gatts_start_service(profile.service_handle);
            break;
        }
        // When add characteristic complete, the event comes:
        case ESP_GATTS_ADD_CHAR_EVT: {
            printf("GATT: ADD_CHAR_EVT: status: %d, attribute handle: %x, service handle: %x, char UUID: %x.\n", param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle, param->add_char.char_uuid.uuid.uuid16);
            // Store characteristic handles for later usage:
            if (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_STEPCOUNT) {
                profile.chars[CHARACTERISTIC_STEPCOUNT_ID].char_handle = param->add_char.attr_handle;
            } else if (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_BPM) {
                profile.chars[CHARACTERISTIC_BPM_ID].char_handle = param->add_char.attr_handle;
            }
            break;
        }
        // When add descriptor complete, the event comes:
        case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
            printf("GATT: ESP_GATTS_ADD_CHAR_DESCR_EVT: status: %d, attribute handle: %d, service handle: %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            break;
        }
        // When disconneting, send advertising information again:
        case ESP_GATTS_DISCONNECT_EVT: {
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }
        // When gatt client connect, the event comes:
        case ESP_GATTS_CONNECT_EVT: {
            printf("GATT: ESP_GATTS_CONNECT_EVT.\n");
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /**
             * For the IOS system, please reference the apple official documents
             * about the BLE connection parameters restrictions.
             */
            conn_params.latency = 0;
            conn_params.max_int = 0x50;                 // max_int = 0x50 * 1.25ms = 100ms
            conn_params.min_int = 0x30;                 // min_int = 0x30 * 1.25ms = 60ms
            conn_params.timeout = 1000;                 // timeout = 1000 * 10ms = 10000ms
            profile.conn_id = param->connect.conn_id;
            // Start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        }
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // If event is register event, store the gatts_if for the profile:
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profile.gatts_if = gatts_if;
        } else {
            printf("GATTS: Register APP failed, APP ID: %04x, status: %d\n", param->reg.app_id, param->reg.status);
            return;
        }
    }
    /**
     * Here call each profile's callback:
     * ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every
     * profile CB function.
     */
    if (gatts_if == ESP_GATT_IF_NONE || gatts_if == profile.gatts_if) {
        if (profile.gatts_cb) {
            profile.gatts_cb(event, gatts_if, param);
        }
    }
}

void app_main()
{
    // Initialize advertising inf.
    adv_params.adv_int_min = 0x20;
    adv_params.adv_int_max = 0x40;
    adv_params.adv_type = ADV_TYPE_IND;
    adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map = ADV_CHNL_ALL;
    adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    // Initialize profile and characteristic permission and property:
    profile.gatts_cb = gatts_profile_event_handler;
    profile.gatts_if = ESP_GATT_IF_NONE;
    profile.chars[CHARACTERISTIC_STEPCOUNT_ID].char_uuid.len = ESP_UUID_LEN_16;
    profile.chars[CHARACTERISTIC_STEPCOUNT_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_STEPCOUNT;
    profile.chars[CHARACTERISTIC_STEPCOUNT_ID].perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
    profile.chars[CHARACTERISTIC_STEPCOUNT_ID].property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
    profile.chars[CHARACTERISTIC_BPM_ID].char_uuid.len = ESP_UUID_LEN_16;
    profile.chars[CHARACTERISTIC_BPM_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_BPM;
    profile.chars[CHARACTERISTIC_BPM_ID].perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
    profile.chars[CHARACTERISTIC_BPM_ID].property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
    
    esp_err_t ret;
    
    // Initialize BLE and Bluedroid:
    btStart();
    ret = esp_bluedroid_init();
    if (ret) {
        printf("%s Init Bluetooth Failed.\n", __func__);
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        printf("%s Enable Bluetooth Failed.\n", __func__);
        return;
    }
    // Set BLE name and broadcast advertising info so that the world can see you:
    esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
    esp_ble_gap_config_adv_data(&adv_data);
    // Register callbacks to handle events of GAP and GATT:
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);

    long last_msg = 0;
    // Send temperature value to registered notification client every 5 seconds via GATT notification.
    while (true) {
        long now = millis();
        if (now - last_msg > 5000) {
        last_msg = now;
        uint8_t bpm = random(0, 50);
        uint8_t step_count = random(0, 50);
        esp_ble_gatts_send_indicate(profile.gatts_if, 
                                    profile.conn_id, 
                                    profile.chars[CHARACTERISTIC_BPM_ID].char_handle,
                                    sizeof(bpm), &bpm, false);
        esp_ble_gatts_send_indicate(profile.gatts_if, 
                                    profile.conn_id, 
                                    profile.chars[CHARACTERISTIC_STEPCOUNT_ID].char_handle,
                                    sizeof(step_count), &step_count, false);
        }
    }
}