#include "mic_i2s.h" 
#include "driver/i2s_std.h"
#include "esp_log.h"
#include <portmacro.h>

static const char *TAG = "MIC_I2S";
i2s_chan_handle_t rx_chan;  // Make sure this is defined globally or passed in

void i2s_mic_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);

    // Corrected: RX is second parameter  
    // i2s_driver_uninstall(I2S_NUM_1); //for debugging only, do not use in production
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg    = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg   = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                          I2S_DATA_BIT_WIDTH_32BIT,
                          I2S_SLOT_MODE_MONO),
        .gpio_cfg   = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = 7,
            .ws   = 16,
            .dout = I2S_GPIO_UNUSED,
            .din  = 15
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_LOGI(TAG, "I2S STD RX channel initialized @16kHz mono");
}


int i2s_mic_read(char *buf, int len) {
    if (!rx_chan) {
        ESP_LOGE(TAG, "I2S channel not initialized");
        return -1;
    }
    size_t bytes = 0;
    esp_err_t err = i2s_channel_read(rx_chan, buf, len, &bytes, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_read error %d", err);
        return -1;
    }
    return (int)bytes;
}
