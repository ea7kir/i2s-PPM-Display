# Log messages - not tested
```
#include "esp_err.h"
#include "esp_log.h"

log_e("err install");
```
--------------------------

# platformio.ini - not tested
```
[env:lilygo-t-display-s3]
platform = https://github.com/platformio/platform-espressif32.git
board = lilygo-t-display-s3
framework = arduino
platform_packages =
    framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32#master
lib_deps = bodmer/TFT_eSPI@^2.5.0

[platformio]
description = i2s PPM Display
```
--------------------------

# Touchpad - not tested
```
touch_pad_deinit()  returns ESP_OK or ESP_FAIL
```

# Bodmer suggested - ok
```
Try adding this line to the setup file:
#define TFT_CS 6
```

# AudioPhonics
Info supplied by AudioPhonics:
```
ADC mode switches          
       dip switches        J0  J1  J2

master +  0  -  -          R   R   L

slave  -  0  -  -          L   L   L

The Jumper J2 is used to obtain half the frequency that you have put on J1 the 22 or 24.

For example, J1 in 24 and J2 in position 1/2 clock your clock, frequency is 12mhz.

With the dip switch 3 on +, you get 96k, and on - 48k.

If you want 192k you have to put J1 on 24 and J2 on full, not on 1/2 clock.
```

# Connections
I'm currently wiring as follows:
```
ADC	name			    ESP32 name      GPIO pin
B		<- yellow ->    .bck_io_num     12  // previously 17
LR		<- blue --->	.ws_io_num      11  // previously 16
D		<- orange ->    .data_in_num    10  // previously 21
CLK		<- green -->    .mck_io_num     3
- see docs for limited choices
```
