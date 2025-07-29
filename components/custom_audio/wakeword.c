#include "esp_wn_models.h" 
//#include "esp_mn_models.h"
#include "wakeword.h"
#include "mic_i2s.h"
#include "esp_log.h"
#include "model_path.h" // Replace esp_srmodel.h with model_path.h
#include "esp_wn_iface.h"
//#include "esp_mn_iface.h"
//#include "esp_mn_speech_commands.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WakeMultinet";

#define SAMPLE_RATE     16000
#define BUFFER_SIZE     512
#define WAKE_MODE       DET_MODE_95
#define CMD_TIMEOUT     40  // ~0.5s @ 16kHz

static wakeword_callback_t ww_callback = NULL;
//static command_callback_t cmd_callback = NULL;

static srmodel_list_t *sr_models = NULL;
static const esp_wn_iface_t *wakenet = NULL;
//static const esp_mn_iface_t *multinet = NULL;
static model_iface_data_t *wn_handle = NULL;
//static model_iface_data_t *mn_handle = NULL;

void wakeword_init_multinet(wakeword_callback_t wake_cb) {//, command_callback_t command_cb
    ww_callback = wake_cb;
    //cmd_callback = command_cb;

    //Note: at end of code, check note

    // Initialize model list from partition
    sr_models = esp_srmodel_init("model");
    if (!sr_models) {
        ESP_LOGE(TAG, "Failed to init model list");
        return;
    }

    // Initialize WakeNet
    char *wn_name = esp_srmodel_filter(sr_models, ESP_WN_PREFIX, NULL);
    if (!wn_name) {
        ESP_LOGE(TAG, "No WakeNet model found");
        esp_srmodel_deinit(sr_models);
        return;
    } 

    int wn_index = esp_srmodel_exists(sr_models, wn_name);
     
    srmodel_data_t *wn_data = sr_models->model_data[wn_index];

    wakenet = esp_wn_handle_from_name(wn_name);
    if (!wakenet) {
        ESP_LOGE(TAG, "Failed to get WakeNet handle for %s", wn_name);
        esp_srmodel_deinit(sr_models);
        return;
    } 
 
    //for debugging 
    ESP_LOGI(TAG, "Using WakeNet: %s", wn_name);
    ESP_LOGI(TAG, "Model data size: %d", wn_data->sizes[0]);
    ESP_LOG_BUFFER_HEX(TAG, wn_data->data[0], 128);
    wn_handle = wakenet->create((const void *)wn_data->data[0], WAKE_MODE);
    if (!wn_handle) {
        ESP_LOGE(TAG, "WakeNet creation failed for %s", wn_name);
        esp_srmodel_deinit(sr_models);
        return;
    }

    // Initialize MultiNet
    // char *mn_name = esp_srmodel_filter(sr_models, ESP_MN_PREFIX, "en"); // or "cn" based on your model
    // if (!mn_name) {
    //     ESP_LOGE(TAG, "No MultiNet model found");
    //     wakenet->destroy(wn_handle);
    //     esp_srmodel_deinit(sr_models);
    //     return;
    // } 

    // multinet = esp_mn_handle_from_name(mn_name);
    // if (!multinet) {
    //     ESP_LOGE(TAG, "Failed to get MultiNet handle for %s", mn_name);
    //     wakenet->destroy(wn_handle);
    //     esp_srmodel_deinit(sr_models);
    //     return;
    // } 

    // mn_handle = multinet->create(mn_name,6000);
    // if (!mn_handle) {
    //     ESP_LOGE(TAG, "MultiNet creation failed for %s", mn_name);
    //     wakenet->destroy(wn_handle);
    //     esp_srmodel_deinit(sr_models);
    //     return;
    // }

    // // Configure MultiNet commands
    // char *lang = multinet->get_language(mn_handle);
    // esp_mn_commands_clear();
    // if (strcmp(lang, ESP_MN_CHINESE) == 0) {
    //     esp_mn_commands_add(1, "da kai kong tiao");
    //     esp_mn_commands_add(2, "guan bi kong tiao");
    // } else if (strcmp(lang, ESP_MN_ENGLISH) == 0) {
    //     esp_mn_commands_add(1, "turn on the light");
    //     esp_mn_commands_add(2, "turn off the light");
    // } else {
    //     ESP_LOGE(TAG, "Invalid language: %s", lang);
    //     wakenet->destroy(wn_handle);
    //     multinet->destroy(mn_handle);
    //     esp_srmodel_deinit(sr_models);
    //     return;
    // }
    // esp_mn_error_t *error_phrases = esp_mn_commands_update();
    // if (error_phrases) {
    //     ESP_LOGE(TAG, "Failed to update commands");
    //     wakenet->destroy(wn_handle);
    //     multinet->destroy(mn_handle);
    //     esp_srmodel_deinit(sr_models);
    //     return;
    // }
    // multinet->print_active_speech_commands(mn_handle);

    ESP_LOGI(TAG, "WakeNet (%s) and MultiNet (%s) initialized", wn_name, "placeholder");//mn_name
}

void wakeword_task(void *arg) {
    int16_t *buffer = calloc(BUFFER_SIZE, sizeof(int16_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Buffer allocation failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        int bytes_read = i2s_mic_read((char *)buffer, BUFFER_SIZE * sizeof(int16_t));
        if (bytes_read <= 0) continue;

        if (wakenet->detect(wn_handle, buffer) == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "Wake word detected");
            if (ww_callback) ww_callback();

            // for (int i = 0; i < CMD_TIMEOUT; ++i) {
            //     bytes_read = i2s_mic_read((char *)buffer, BUFFER_SIZE * sizeof(int16_t));
            //     if (bytes_read <= 0) continue;

                // esp_mn_state_t mn_state = multinet->detect(mn_handle, buffer);
                // if (mn_state == ESP_MN_STATE_DETECTED) {
                //     esp_mn_results_t *res = multinet->get_results(mn_handle);
                //     if (res && res->num > 0) {
                //         ESP_LOGI(TAG, "Command recognized: %s (ID: %d)", res->string, res->command_id[0]);
                //         if (cmd_callback) cmd_callback(res->string);
                //         multinet->clean(mn_handle);
                //         break;
                //     }
                // } else if (mn_state == ESP_MN_STATE_TIMEOUT) {
                //     ESP_LOGI(TAG, "Command detection timeout");
                //     break;
                // }
            //}
        }
    }

    // Cleanup (not usually reached)
    free(buffer);
    wakenet->destroy(wn_handle);
    // multinet->destroy(mn_handle);
    esp_srmodel_deinit(sr_models);
    vTaskDelete(NULL);
} 
 
/*  
1.
// Initialize I2S microphone!!!!!!!!!!DO NOT USE HERE AS ALREADY USED IN MAIN
    //i2s_mic_init();!!!!!!! will cause double initialization error 
                            W (1987) i2s_platform: i2s controller 1 has been occupied by i2s_driver
                            E (1987) i2s_common: i2s_new_channel(996): no available channel found 
*/ 