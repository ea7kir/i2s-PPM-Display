/*
 *  i2s-PPM-Display
 *  Copyright (c) 2023 Michael Naylor EA7KIR (https://michaelnaylor.es)
 */

#include "I2S_Input.hpp"

#include <Arduino.h>
#include <driver/i2s.h>

#define EA7_B_to_bck_io GPIO_NUM_12     /* yellow */
#define EA7_LR_to_ws_io GPIO_NUM_11     /* blue   */
#define EA7_D_to_data_in GPIO_NUM_10    /* orange */
#define EA7_CLK_to_mck_io GPIO_NUM_3    /* green  */

// NOTE: ESP32 supports setting MCK only on GPIO_NUM_1, GPIO_NUM_2 or GPIO_NUM_3
//  However, GPIO_NUM_1 is now being used to enable MS mode.

#define EA7_SampleRate 48000

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

// Define input buffer length
#define EA7_BufferLen 64

struct {
    int16_t left;
    int16_t right;
} sBuffer[EA7_BufferLen];

void I2S_Init()
{
    //log_i("CoreID %i", (int)xPortGetCoreID());

    // Set up I2S Processor configuration
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = EA7_SampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 8,
        .dma_buf_len = EA7_BufferLen,
        };

    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK)
    {
        //log_e("i2s_driver_install != ESP_OK");
    }

    // Set I2S pin configuration
    const i2s_pin_config_t pin_config = {
        .mck_io_num = EA7_CLK_to_mck_io,
        .bck_io_num = EA7_B_to_bck_io,
        .ws_io_num = EA7_LR_to_ws_io,
        .data_out_num = I2S_PIN_NO_CHANGE, // -1,
        .data_in_num = EA7_D_to_data_in
        };

    if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK)
    {
        //log_e("i2s_setpin != ESP_OK");
    }

    // if (i2s_start(I2S_PORT) != ESP_OK) // apparently not neccessary.
    // {
    //     log_e("i2s_start != ESP_OK");
    // }
}

void I2S_Read(rawSampleStruct_t &rawSample) // perhaps better to just return a rawSampleStruct_t
{
    // Get I2S data and place in data buffer
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, &sBuffer, EA7_BufferLen, &bytesIn, portMAX_DELAY);

    if (result == ESP_OK)
    {
        int16_t samples_read = bytesIn / 8;
        //log_i("samples_read %i", samples_read);

        rawSample.left = 0;
        rawSample.right = 0;

        int16_t left = 0;
        int16_t right = 0;

        if (samples_read > 0)
        {
            for (int16_t i = 0; i < samples_read; i++)
            {
                // The idea here is 'rectify' and reduce the data rate.
                // This appears to do the job, but is it the correct way?
                // A better way might be to average the samples, but should
                // this be before taking the absolute values or not?
                left = sBuffer[i].left;
                right = sBuffer[i].right;
                // 'rectify'
                if (left < 0) left = -left;
                if (right < 0) right = -right;
                // save the peaks
                if (left > rawSample.left) rawSample.left = left;
                if (right > rawSample.right) rawSample.right = right;
            }
        }
    }
}
