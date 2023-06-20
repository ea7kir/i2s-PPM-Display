/*
 *  i2s-PPM-Display
 *  Copyright (c) 2023 Michael Naylor EA7KIR (https://michaelnaylor.es)
 */

// https://www.freertos.org/a00106.html

#include <Arduino.h>

// #include "esp_err.h"
// #include "esp_log.h"

#include "PPM_Meter.hpp"
#include "I2S_Input.hpp"

TaskHandle_t ReadI2S_handle = NULL;
TaskHandle_t UpdateScreen_handle = NULL;
QueueHandle_t xStructQueue = NULL;

struct AMessage
{
    int16_t left;
    int16_t right;
};

void Task_ReadI2S(void *pvParameters)
{
    //log_i("CoreID %i", (int)xPortGetCoreID());

    rawSampleStruct_t rawSample;
    struct AMessage xMessage;

    I2S_Init();
    vTaskDelay(500 / portTICK_RATE_MS);

    while (1)
    {
        I2S_Read(rawSample);

        // log_i("count %i, rawL %i, rawR %i", (int)rawSample.count, (int)rawSample.left[0], (int)rawSample.right[0]);

        xMessage.left = rawSample.left;
        xMessage.right = rawSample.right;

        xQueueSend(xStructQueue, (void *)&xMessage, (TickType_t)0);

        vTaskDelay(0);
    }
}

void Task_UpdateBallistiics(void *pvParameters)
{
    //log_i("CoreID %i", (int)xPortGetCoreID());

    struct AMessage xRxedStructure;

    while (1)
    {
        if (xStructQueue != NULL)
        {
            if (xQueueReceive(xStructQueue, &(xRxedStructure), (TickType_t)10) == pdPASS)
            {
                // log_i("count %i", (int)xRxedStructure.count);
                // log_i("count %i, rawL %i, rawR %i", (int)xRxedStructure.count, (int)xRxedStructure.left[i], (int)xRxedStructure.right[i]);
                PPM_ProcessAndStore(xRxedStructure.left, xRxedStructure.right);
            }
        }
        else
        {
            // log_e("xStructQueue == NULL");
        }

        vTaskDelay(0);
    }
}

void setup()
{
    // Serial.begin(115200);
    // delay(1000);
    // // esp_log_level_set("*", ESP_LOG_ERROR);    // set all components to ERROR level
    // esp_log_level_set("wifi", ESP_LOG_WARN);  // enable WARN logs from WiFi stack
    // esp_log_level_set("dhcpc", ESP_LOG_INFO); // enable INFO logs from DHCP client

    // log_e("level is %s", "1 error");
    // log_w("level is %s", "2 waring");
    // log_i("level is %s", "3 info");
    // log_d("level is %s", "4 degug");
    // log_v("level is %s", "5 verbose");

    // xMessage.count = 0;
    // for (uint i = 0; i < EA7_BufferLen; i++)
    // {
    //     xMessage.left[i] = 0;
    //     xMessage.right[i] = 0;
    // }

    xStructQueue = xQueueCreate(10, sizeof(AMessage));

    if ((xStructQueue == NULL))
    {
        //log_e("xStructQueue == NULL");
    }

    // launch the Tasks in order

    xTaskCreatePinnedToCore(
        Task_PPM_UpdateNeedles, // function name
        "Task_PPM_UpdateNeedles",    // task name
        1500,           // stack size NOTE: 1200 appears to be minimum!
        NULL,           // task parameters
        1,              // task priority (higher number is higher priority)
        NULL,           // task handle
        0               // cpu
    );

    xTaskCreatePinnedToCore(
        Task_UpdateBallistiics, // function name
        "Task_UpdateBallistiics",    // task name
        1500,           // stack size NOTE: 1200 appears to be minimum!
        NULL,           // task parameters
        1,              // task priority (higher number is higher priority)
        NULL,           // task handle
        0               // cpu
    );

    xTaskCreatePinnedToCore(
        Task_ReadI2S, // function name
        "Task_ReadI2S",    // task name
        1500,         // stack size
        NULL,         // task parameters
        1,            // task priority (higher number is higher priority)
        NULL,         // task handle
        1             // cpu
    );
}

void loop()
{
    // do nothing
}
