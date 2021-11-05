
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

#include "led.h"
#include "mpu6050_i2c.h"
#include "fishBiteDetector.h"

#include "ble_demo.h"

#include "sdkconfig.h"

#define MAIN_TAG    "MAIN_FN"

static void checkFishBite(void* arg)
{
    while (1)
    {
        mpu6050_update();
        double az = mpu6050_get_accelZ();
        fbd_update(az/mpu6050_get_accel_resolution());
        
        // if (fbd_check_fishbite() == 1)
        // {
        //     //ESP_LOGI(GATTS_TAG, "fish bite detected, gz = %f", gz/16384);

        //     uint8_t val[4] = {0,1,2,3};
        //     ble_sendIndication(PROFILE_C_APP_ID, val);
        // }
        // else
        // {
        //     //ESP_LOGI(GATTS_TAG, "fish bite NOT detected, gz = %f", gz/16384);
        // }
        
        led_debug_toggle();
        vTaskDelay(50/ portTICK_PERIOD_MS);
    }
}

void onFishBiteDetected(void)
{
    uint8_t val[4] = {0,1,2,3};
    ble_sendIndication(PROFILE_C_APP_ID, val);

    led_set_colorRed();

    ESP_LOGI(MAIN_TAG, "FISH BITE DETECTED HANDLER CALLED");
}

void onFishBiteRelease(void)
{
    led_set_colorGrn();
    
    ESP_LOGI(MAIN_TAG, "FISH BITE RELEASE HANDLER CALLED");
}

void onSensitivityChanged(uint8_t value)
{
    if (value <= 0)
    {
        value = 1;
    }

    fbd_sensitivity((double)value * 0.01);
    
    ESP_LOGI(MAIN_TAG, "Sensitivity: %d", value);
}

void onBrightnessChanged(uint8_t value)
{
    if (value <= 0){
        value = 0;
    }   
    if (value > 100){
        value = 100;
    }

    led_set_brightness((double)value);
}

void app_main(void)
{
    esp_err_t ret;
    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    led_init();
    led_debug_init();
    mpu6050_init();
    
    // fish bite detect library initialize
    fbd_init();
    fbd_set_detect_handler(onFishBiteDetected); // function to call when fishbite is detected
    fbd_set_release_handler(onFishBiteRelease); // function to call when fishbite release

    ble_init();
    ble_set_onSensitivityChange(onSensitivityChanged);
    ble_set_onBrightnessChanged(onBrightnessChanged);

    xTaskCreate(checkFishBite, "FishBite_task", 4096, NULL, 10, NULL);

}