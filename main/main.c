/**
 * @file main.c
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * Gabriel Boni Vicari (133192@upf.br)
 * GEPID - Grupo de Pesquisa em Cultura Digital (http://gepid.upf.br/)
 * Universidade de Passo Fundo (http://www.upf.br/)
 *
 * @copyright 2018 Angelo Elias Dalzotto, Gabriel Boni Vicari
 *
 * @brief Main file for the MYB project for the ESP-IDF.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <driver/i2c.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "mpu6050/mpu6050.h"
#include "max30100/max30100.h"

#define PI              3.14159265358979323846f
#define AVG_BUFF_SIZE   20
#define SAMPLE_SIZE     2000
#define I2C_SDA         26
#define I2C_SCL         25
#define I2C_FREQ        400000
#define I2C_PORT        I2C_NUM_0

float self_test[6] = {0, 0, 0, 0, 0, 0};
float accel_bias[3] = {0, 0, 0};
float gyro_bias[3] = {0, 0, 0};

max30100_config_t max30100;
max30100_data_t result;

esp_err_t i2c_master_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA;
    conf.scl_io_num = I2C_SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FREQ;
	i2c_param_config(I2C_PORT, &conf);

	return (i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
}

void step_counter()
{
    mpu6050_acceleration_t accel;
    mpu6050_rotation_t gyro;
    uint64_t now_time = 0, prev_time = 0;
    int16_t temp;
    int8_t range = 0;
    float accel_g_x, accel_g_y, accel_g_z, accel_int_x, accel_int_y, accel_int_z;
    float gyro_ds_x, gyro_ds_y, gyro_ds_z, accel_res, gyro_res, temp_c;
    int accel_x_avg_buff[AVG_BUFF_SIZE];
    int accel_y_avg_buff[AVG_BUFF_SIZE];
    int accel_z_avg_buff[AVG_BUFF_SIZE];
    int accel_x_avg_buff_count = 0;
    int accel_y_avg_buff_count = 0;
    int accel_z_avg_buff_count = 0;
    int accel_x_avg, accel_y_avg, accel_z_avg;
    int min_reg_accel_x = 0, min_reg_accel_y = 0, min_reg_accel_z = 0;
    int max_reg_accel_x = 0, max_reg_accel_y = 0, max_reg_accel_z = 0;
    int min_current_accel_x, min_current_accel_y, min_current_accel_z;
    int max_current_accel_x, max_current_accel_y, max_current_accel_z;
    int dy_thres_accel_x = 0, dy_thres_accel_y = 0, dy_thres_accel_z = 0;
    int dy_chan_accel_x, dy_chan_accel_y, dy_chan_accel_z;
    int sample_new = 0, sample_old = 0, return_samples = 0;
    int step_size = 200, step_count = 0, count_accel = 0;
    int active_axis = 0, interval = 500000;

    while (true) {
        if (!mpu6050_get_int_dmp_status()) {
            mpu6050_get_acceleration(&accel);

            range = mpu6050_get_full_scale_accel_range();
            accel_res = mpu6050_get_accel_res(range);

            accel_g_x = (float) accel.accel_x * accel_res - accel_bias[0];
            accel_g_y = (float) accel.accel_y * accel_res - accel_bias[1];
            accel_g_z = (float) accel.accel_z * accel_res - accel_bias[2];

            if (accel_g_x < 0)
                accel_g_x *= -1;
            if (accel_g_y < 0)
                accel_g_y *= -1;
            if (accel_g_z < 0)
                accel_g_z *= -1;

            mpu6050_get_rotation(&gyro);

            range = mpu6050_get_full_scale_gyro_range();
            gyro_res = mpu6050_get_gyro_res(range);

            gyro_ds_x = (float) gyro.gyro_x * gyro_res - gyro_bias[0];
            gyro_ds_y = (float) gyro.gyro_y * gyro_res - gyro_bias[1];
            gyro_ds_z = (float) gyro.gyro_z * gyro_res - gyro_bias[2];

            temp = mpu6050_get_temperature();
            temp_c = (float) temp / 340.0 + 36.53;

            mpu6050_madgwick_quaternion_update
            (
                accel_g_x,
                accel_g_y,
                accel_g_z,
                gyro_ds_x * PI / 180.0f,
                gyro_ds_y * PI / 180.0f,
                gyro_ds_z * PI / 180.0f
            );

            accel_int_x = 1000 * accel_g_x;
            accel_int_y = 1000 * accel_g_y;
            accel_int_z = 1000 * accel_g_z;

            accel_x_avg_buff[accel_x_avg_buff_count] = accel_int_x;
            accel_x_avg_buff_count++;
            accel_x_avg_buff_count %= AVG_BUFF_SIZE;
            accel_x_avg = 0;

            for (int i = 0; i < AVG_BUFF_SIZE; i++)
                accel_x_avg += accel_x_avg_buff[i];
            
            accel_x_avg /= AVG_BUFF_SIZE;

            accel_y_avg_buff[accel_y_avg_buff_count] = accel_int_y;
            accel_y_avg_buff_count++;
            accel_y_avg_buff_count %= AVG_BUFF_SIZE;
            accel_y_avg = 0;

            for (int i = 0; i < AVG_BUFF_SIZE; i++)
                accel_y_avg += accel_y_avg_buff[i];
            
            accel_y_avg /= AVG_BUFF_SIZE;

            accel_z_avg_buff[accel_z_avg_buff_count] = accel_int_z;
            accel_z_avg_buff_count++;
            accel_z_avg_buff_count %= AVG_BUFF_SIZE;
            accel_z_avg = 0;

            for (int i = 0; i < AVG_BUFF_SIZE; i++)
                accel_z_avg += accel_z_avg_buff[i];
            
            accel_z_avg /= AVG_BUFF_SIZE;

            now_time = esp_timer_get_time();

            if (now_time - prev_time >= interval) {
                prev_time = now_time;
            
                min_current_accel_x = min_reg_accel_x;
                max_current_accel_x = max_reg_accel_x;
                dy_thres_accel_x = (min_current_accel_x + max_current_accel_x) / 2;
                dy_chan_accel_x = (max_current_accel_x - min_current_accel_x);
                min_reg_accel_x = accel_x_avg;
                max_reg_accel_x = accel_x_avg;
                min_current_accel_y = min_reg_accel_y;
                max_current_accel_y = max_reg_accel_y;
                dy_thres_accel_y = (min_current_accel_y + max_current_accel_y) / 2;
                dy_chan_accel_y = (max_current_accel_y - min_current_accel_y);
                min_reg_accel_y = accel_y_avg;
                max_reg_accel_y = accel_y_avg;
                min_current_accel_z = min_reg_accel_z;
                max_current_accel_z = max_reg_accel_z;
                dy_thres_accel_z = (min_current_accel_z + max_current_accel_z) / 2;
                dy_chan_accel_z = (max_current_accel_z - min_current_accel_z);
                min_reg_accel_z = accel_z_avg;
                max_reg_accel_z = accel_z_avg;
                
                if (dy_chan_accel_x >= dy_chan_accel_y && dy_chan_accel_x >= dy_chan_accel_z) {
                    if (active_axis != 0) {
                        sample_old = 0;
                        sample_new = accel_x_avg;
                    }
                    active_axis = 0;
                } else if (dy_chan_accel_y >= dy_chan_accel_x && dy_chan_accel_y >= dy_chan_accel_z) {
                        if (active_axis != 1) {
                        sample_old = 0;
                        sample_new = accel_y_avg;
                    }
                    active_axis = 1;
                } else {
                    if (active_axis != 2) {
                        sample_old = 0;
                        sample_new = accel_z_avg;
                    }
                    active_axis = 2;
                }

            } else if (now_time < 500) {
                if (min_reg_accel_x > accel_x_avg)
                    min_reg_accel_x = accel_x_avg;
                if (max_reg_accel_x < accel_x_avg)
                    max_reg_accel_x = accel_x_avg;
                if (min_reg_accel_y > accel_y_avg)
                    min_reg_accel_y = accel_y_avg;
                if (max_reg_accel_y < accel_y_avg)
                    max_reg_accel_y = accel_y_avg;
                if (min_reg_accel_z > accel_z_avg)
                    min_reg_accel_z = accel_z_avg;
                if (max_reg_accel_z < accel_z_avg)
                    max_reg_accel_z = accel_z_avg;
            }
            
            sample_old = sample_new;
            switch (active_axis) {
                case 0:
                    if (accel_x_avg - sample_old > step_size || accel_x_avg - sample_old < -step_size) {
                        sample_new = accel_x_avg;
                        if (sample_old > dy_thres_accel_x && sample_new < dy_thres_accel_x)
                            step_count++;
                    }
                    break;
                case 1:
                    if (accel_y_avg - sample_old > step_size || accel_y_avg - sample_old < -step_size) {
                        sample_new = accel_y_avg;
                        if (sample_old > dy_thres_accel_y && sample_new < dy_thres_accel_y)
                            step_count++;
                    }
                    break;
                case 2:
                    if (accel_z_avg - sample_old > step_size || accel_z_avg - sample_old < -step_size) {
                        sample_new = accel_z_avg;
                        if (sample_old > dy_thres_accel_z && sample_new < dy_thres_accel_z)
                            step_count++;
                    }
                    break;
            }

            ESP_LOGI(mpu6050_get_tag(), "X (Average): %d | Y (Average): %d", accel_x_avg, accel_y_avg);
            ESP_LOGI(mpu6050_get_tag(), "Temperature: %.3f | Y: %d | Step Counter: %d", temp_c, accel_y_avg, step_count);

            FILE* file = fopen("/spiffs/stepcount.csv", "a");
            if (file == NULL) {
                ESP_LOGE(mpu6050_get_tag(), "Failed to open file stepcount.csv for writing.");
                return;
            }
            fprintf(file, "%d,", step_count);
            fclose(file);
            
            if (count_accel < SAMPLE_SIZE || (count_accel == SAMPLE_SIZE && return_samples == 1))
                count_accel++;
            
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void bpm_counter(void* param)
{
    max30100_data_t result;
    float bpm_final = 0;
    int bpm_count = 0;

    while (true) {
        max30100_update(&max30100, &result);
        if (result.pulse_detected)
           ESP_LOGI(max30100_get_tag(), "BPM: %f | SpO2: %f%%", result.heart_bpm, result.spO2);

        FILE* file = fopen("/spiffs/bpm.csv", "a");
        if (file == NULL) {
            ESP_LOGE(max30100_get_tag(), "Failed to open file bpm.csv for writing.");
            return;
        }
        fprintf(file, "%f,", bpm_final);
        fclose(file);
            
        file = fopen("/spiffs/sp02.csv", "a");
        if (file == NULL) {
            ESP_LOGE(max30100_get_tag(), "Failed to open file sp02.csv for writing.");
            return;
        }
        fprintf(file, "%f,", result.spO2);
        fclose(file);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    esp_err_t ret;

    i2c_master_init();

    esp_vfs_spiffs_conf_t spiffs_config = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK) {    
        if (ret == ESP_FAIL)
            ESP_LOGE("SPIFFS", "Failed to mount or format filesystem.");
        else if (ret == ESP_ERR_NOT_FOUND)
            ESP_LOGE("SPIFFS", "Failed to find SPIFFS partition.");
        else
            ESP_LOGE("SPIFFS", "Failed to initialize SPIFFS (%s).", esp_err_to_name(ret));
    }
    else
        ESP_LOGI("SPIFFS", "Initialized.");

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to get SPIFFS partition information (%s).", esp_err_to_name(ret));
    } else {
        ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d.", total, used);
    }

    ESP_LOGI(mpu6050_get_tag(), "Device ID: %d.", mpu6050_get_device_id());

    mpu6050_self_test(self_test);
    ESP_LOGI(mpu6050_get_tag(), "Device performing self-test.");

    if (self_test[0] < 1.0f && self_test[1] < 1.0f && self_test[2] < 1.0f &&
        self_test[3] < 1.0f && self_test[4] < 1.0f && self_test[5] < 1.0f) {
        mpu6050_reset();
        mpu6050_calibrate(accel_bias, gyro_bias);
        ESP_LOGI(mpu6050_get_tag(), "Device being calibrated.");
        mpu6050_init();
        ESP_LOGI(mpu6050_get_tag(), "Device initialized.");
        xTaskCreate(step_counter, "StepCounter", 10000, (void*) 0, 10, NULL);
    } 
    else
        ESP_LOGI(mpu6050_get_tag(), "Device did not pass self-test.");

    max30100_init
    (
        &max30100,
        I2C_PORT,
        MAX30100_DEFAULT_OPERATING_MODE,
        MAX30100_DEFAULT_SAMPLING_RATE,
        MAX30100_DEFAULT_LED_PULSE_WIDTH,
        MAX30100_DEFAULT_IR_LED_CURRENT,
        MAX30100_DEFAULT_START_RED_LED_CURRENT,
        MAX30100_DEFAULT_MEAN_FILTER_SIZE,
        5,
        true,
        false
    );
    ESP_LOGI(max30100_get_tag(), "Device ID: %d.", max30100_get_device_id());
    ESP_LOGI(max30100_get_tag(), "Device initialized.");
    xTaskCreate(bpm_counter, "BPMCounter", 10000, NULL, 1, NULL);

    esp_vfs_spiffs_unregister(NULL);
}