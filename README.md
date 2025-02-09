# EI_ESP32_CAM_SERVER

## Preparation

Arduino IDE version:

### File upload

We need to upload our files to esp-32 via [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)

1. Go to the [releases](https://github.com/earlephilhower/arduino-littlefs-upload/releases) page and click the `.vsix` file to download.
2. Move the file to Plugins Directory

    ```bash
    # Find the .arduinoIDE directory
    cd ~/.arduinoIDE/
    pwd
    # Create plugins dir, if it's not there
    mkdir plugins
    # Copt the files, in my case it was downloaded in Downloads dir
    cd \
    cd Downloads
    cp arduino-littlefs-upload-x.x.x.vsix ~/.arduinoIDE/plugins/
    ```

3. Quit & reopen Arduino IDE
4. Pressing `CMD` + `SHIFT` + `P`, will open commands palette of Arduino IDE
5. Type in `Upload LittleFS` and the full command (`Upload LittleFS to Pico/ESP8266/ESP32`) will show up. Hit `ENTER`
6. All the contents from [`data/`](data/) will not be transferred to the fs of ESP32 
   > Make sure Serial Monitor is closed

### Credentials Settings

Copy [credentials.h.template](credentials.h.template) to a new file called `credentials.h` and update it's contents:

```c++
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PWD";

#endif // CREDENTIALS_H
```

Update wifi credentials and your [Edge Impulse API key](https://docs.edgeimpulse.com/reference/edge-impulse-api/edge-impulse-api) (WIP)

### Camera Settings

Pick a esp32 camera module based on the [camera_pins.h](camera_pins.h) and use only one in [camera_init.h](camera_init.h)

```c++
// Define camera model before including camera_pins.h
// e.g.: We are using cheap AI Thinker Cam
#define CAMERA_MODEL_AI_THINKER 1
```



### Server Port Settings

---

## Acknowledgement

```txt
Saurabh Datta
zigzag.is
Feb 2025
datta@zigzag.is
hi@dattasaurabh,com
```
