/*
 *  i2s-PPM-Display
 *  Copyright (c) 2023 Michael Naylor EA7KIR (https://michaelnaylor.es)
 */

#include <Arduino.h>

#include "PPM_Meter.hpp"
#include <TFT_eSPI.h>
#include <mutex>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite theSprite = TFT_eSprite(&tft);
TFT_eSprite backgroundSprite = TFT_eSprite(&tft);

const int pivotX = 160, pivotY = 250;
const int needleRadius = 190;
const int markLength = needleRadius / 7;
const int markRadius = needleRadius - markLength / 2;
const int shortMarkLength = markLength * 2 / 3;
const int textGap = 10;
const int textRadius = needleRadius + markLength + textGap;

const float Pi = 3.14159265358979323846264;
const float PiOver180 = Pi / 180.0;

static struct
{
    std::mutex mu;
    float left = 0;
    float right = 0;
} storedIntegratedValue;

typedef struct
{
    float startX, startY, tipX, tipY, txtX, txtY;
    String label;
} markData_struct;

std::array<markData_struct, 9> markData;

void PPM_Init()
{
    // create an array for the scale marks and numbers 1 to 7
    // these 'nice numbers' create a 91 degree meter scale
    // with 13 degrees per 4 decibels between each major division
    std::array<float, 9> angle = {
        -45.5 * PiOver180, // min -32
        -39.0 * PiOver180, // 1   -30
        -26.0 * PiOver180, // 2   -26
        -13.0 * PiOver180, // 3   -22
        0.0 * PiOver180,   // 4   -18
        13.0 * PiOver180,  // 5   -14
        26.0 * PiOver180,  // 6   -10
        39.0 * PiOver180,  // 7   - 6
        45.5 * PiOver180,  // max - 4
    };
    const String label[] = {"", "1", "2", "3", "4", "5", "6", "7", ""};
    int length;
    for (int i = 0; i < 9; i++)
    {
        if (i == 0 || i == 8)
            length = shortMarkLength;
        else
            length = markLength;
        markData[i].startX = pivotX + round(markRadius * sin(angle[i]));
        markData[i].startY = pivotY - round(markRadius * cos(angle[i]));
        markData[i].tipX = pivotX + round((markRadius + length) * sin(angle[i]));
        markData[i].tipY = pivotY - round((markRadius + length) * cos(angle[i]));
        markData[i].txtX = pivotX + round((textRadius)*sin(angle[i]));
        markData[i].txtY = pivotY - round((textRadius)*cos(angle[i]));
        markData[i].label = label[i];
    }
    // init TFT and Sprite
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    theSprite.createSprite(320, 170);
    theSprite.setViewport(0, 0, 320, 170);

    //  clear the sprite
    backgroundSprite.createSprite(320, 170);
    backgroundSprite.setViewport(0, 0, 320, 170);
    backgroundSprite.fillSprite(TFT_BLACK);

    // draw the scale
    for (int i = 0; i < 9; i++)
    {
        backgroundSprite.drawWideLine(markData[i].startX, markData[i].startY, markData[i].tipX, markData[i].tipY, 3, TFT_WHITE);
        backgroundSprite.setCursor(markData[i].txtX, markData[i].txtY, 2);
        backgroundSprite.print(markData[i].label);
    }

}

float integratedValueFromRaw(float oldIntegratedValue, int16_t rawInt16Value)
{
    const float ATTACK = 0.15;       // greater is faster was 0.35
    const float RELEASE = 0.9990;    // greater is slower was 0.9991

    // raw is already 'rectified', so convert to float
    float rawFloatValue = rawInt16Value;

    // integrate to required PPM attack/recover balistic
    if (rawFloatValue > oldIntegratedValue)
        return (rawFloatValue * ATTACK) + (oldIntegratedValue * (1 - ATTACK));

    return (rawFloatValue * (1 - RELEASE)) + (oldIntegratedValue * RELEASE);
}

// called from Task
void PPM_ProcessAndStore(int16_t rawL, int16_t rawR)
{
    //log_i("rawL %i, rawR %i", (int)rawL, (int)rawR);
    static float integratedL = 0, integratedR = 0;

    integratedL = integratedValueFromRaw(integratedL, rawL);
    integratedR = integratedValueFromRaw(integratedR, rawR);

    storedIntegratedValue.mu.lock();
    storedIntegratedValue.left = integratedL;
    storedIntegratedValue.right = integratedR;
    storedIntegratedValue.mu.unlock();
}

// void plotNeedle(float angle, int color) // angle in radians
// {
//     // const float ppmMin = -45.5 * PiOver180, ppmMax = 45.5 * PiOver180;
//     // if (angle < ppmMin)
//     //     angle = ppmMin;
//     // else if (angle > ppmMax)
//     //     angle = ppmMax;
//     int tipX = pivotX + round(needleRadius * sin(angle));
//     int tipY = pivotY - round(needleRadius * cos(angle));
//     theSprite.drawWideLine(pivotX, pivotY, tipX, tipY, 3, color);
// }

float angleFromIntegratedValue(float integratedValue)
{
    // convert to dbfs
    float normalizedValue = integratedValue / 32768;
    float dbfs = 20 * log10(normalizedValue);
    if (dbfs < -32)
        dbfs = -32;
    else if (dbfs > -4)
        dbfs = -4;

    // convert to radians
    const float in_min = -32, in_max = -4, out_min = -45.5, out_max = 45.5;
    const float run = in_max - in_min;
    const float rise = out_max - out_min;
    const float delta = dbfs - in_min;
    float result = (delta * rise) / run + out_min;
    float angle = result * PiOver180;

    return angle;
}

void Task_PPM_UpdateNeedles(void *pvParameters)
{
    float integratedL, integratedR;
    float slowAngleL = -20, slowAngleR = 20;
    int tipX, tipY;

    //log_i("CoreID %i", (int)xPortGetCoreID());

    // int counter = 0;
    PPM_Init();
    vTaskDelay(500 / portTICK_RATE_MS);

    static bool isReady = true;

    while (1)
    {
        if (isReady)
        {
            isReady = false;

            //log_i("tick %i", ++counter);

            //  clear the sprite
            // theSprite.fillSprite(TFT_BLACK);

            // // draw the scale
            // for (int i = 0; i < 9; i++)
            // {
            //     theSprite.drawWideLine(markData[i].startX, markData[i].startY, markData[i].tipX, markData[i].tipY, 3, TFT_WHITE);
            //     theSprite.setCursor(markData[i].txtX, markData[i].txtY, 2);
            //     theSprite.print(markData[i].label);
            // }

            backgroundSprite.pushToSprite(&theSprite, 0, 0);

            storedIntegratedValue.mu.lock();
            integratedL = storedIntegratedValue.left;
            integratedR = storedIntegratedValue.right;
            storedIntegratedValue.mu.unlock();

            // Frankly, the balistics looked nicer when I was integrating the angles (not the raw values).

            slowAngleL = angleFromIntegratedValue(integratedL);
            slowAngleR = angleFromIntegratedValue(integratedR);

            //log_i("slowAngleL %f, slowAngleR %f", slowAngleL, slowAngleR);

            // draw the needles

            tipX = pivotX + round(needleRadius * sin(slowAngleR));
            tipY = pivotY - round(needleRadius * cos(slowAngleR));
            theSprite.drawWideLine(pivotX, pivotY, tipX, tipY, 3, TFT_DARKGREEN);

            tipX = pivotX + round(needleRadius * sin(slowAngleL));
            tipY = pivotY - round(needleRadius * cos(slowAngleL));
            theSprite.drawWideLine(pivotX, pivotY, tipX, tipY, 3, TFT_RED);

            // plotNeedle(slowAngleR, TFT_DARKGREEN);
            // plotNeedle(slowAngleL, TFT_RED);

            // publish to the screen
            theSprite.pushSprite(0, 0);

            vTaskDelay(5 / portTICK_RATE_MS);

            isReady = true;
        }
    }
}
