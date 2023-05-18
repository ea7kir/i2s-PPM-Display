/*
 *  i2s-PPM-Display
 *  Copyright (c) 2023 Michael Naylor EA7KIR (https://michaelnaylor.es)
 */

#pragma once

#include <Arduino.h>

void PPM_Init();
void PPM_ProcessAndStore(int16_t rawL, int16_t rawR);
void Task_PPM_UpdateNeedles(void *pvParameters);
