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