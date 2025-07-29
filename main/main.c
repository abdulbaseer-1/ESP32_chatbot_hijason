/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h" 
#include "mic_i2s.h"
#include "speaker_i2s.h"
#include "wakeword.h"
#include "Mymqtt_client.h"
#include "wifi.h" 
#include "esp_log.h"  
//To add later for commercial product  
//config.h
//#include "system/ota.h" // 
 
  
#define TAG "MAIN"  
 
static esp_mqtt_client_handle_t mqtt_client = NULL; 
static const char *mqtt_url = "mqtt://192.168.1.10:1883"; // Replace with your MQTT broker URL

 
void send_audio_to_server(void* param)
{
    const int record_time_sec = 5;
    const int sample_rate = 16000;
    const int bytes_per_sample = 2;
    const int channels = 1;

    int buffer_size = sample_rate * record_time_sec * bytes_per_sample * channels;
    char *audio_buffer = malloc(buffer_size);

    if (!audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer"); 
        vTaskDelete(NULL);  // delete the task if allocation fails
        return;
    }

    int bytes_read = i2s_mic_read(audio_buffer, buffer_size);
    if (bytes_read > 0) {
        ESP_LOGI(TAG, "Read %d bytes of audio", bytes_read);
        // nmqtt publish_audio will handle the MQTT publish
        mqtt_publish_audio(mqtt_client, "/audio/request", audio_buffer, bytes_read);
    } else {
        ESP_LOGE(TAG, "Failed to read audio");
    }

    free(audio_buffer); 
     
    vTaskDelete(NULL);  // delete the task once done
}

static void wakeword_detected_callback() {
    ESP_LOGI(TAG, "Wake word callback triggered");
    xTaskCreate(&send_audio_to_server, "send_audio_to_server", 8192, NULL, 5, NULL);
} 

void mqtt_message_handler(const char *topic, const char *data, int len) {
    ESP_LOGI("MQTT_CB", "Received topic: %s", topic);
    ESP_LOGI("MQTT_CB", "Payload: %.*s", len, data);
}

void app_main(void)
{ 
    ESP_LOGI(TAG, "Starting application");

    // Initialize Wi-Fi 
    wifi_init_sta(); 

    // Initialize MQTT client 
    mqtt_client = mqtt_init_client(mqtt_url, mqtt_message_handler);

    if (!mqtt_client) {
        ESP_LOGE(TAG, "MQTT client init failed");
    }


    // Initialize I2S microphone 
    ESP_LOGI(TAG, "Starting audio recording...");
    i2s_mic_init(); 
    i2s_speaker_init(); 
    i2s_speaker_play_sine_wave();

    // Initialize wake-word detection
    wakeword_init_multinet(wakeword_detected_callback);  //, NULL // Init WakeNet

    xTaskCreate(&wakeword_task, "wakeword_task", 8192, NULL, 5, NULL);

    // Initialize OTA
    // ota_init();

    // Periodically check for OTA updates
    // Consider creating a separate task for OTA checks
 
//-------------------------------------------------------------------------------------------------------------------------------
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
     
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
