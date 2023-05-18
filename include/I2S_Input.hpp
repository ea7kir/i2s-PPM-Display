/*
 *  i2s-PPM-Display
 *  Copyright (c) 2023 Michael Naylor EA7KIR (https://michaelnaylor.es)
 */

#pragma once

#include <Arduino.h>

typedef struct {
    int16_t left;
    int16_t right;
} rawSampleStruct_t;

void I2S_Init();
void I2S_Read(rawSampleStruct_t &rawSample);
