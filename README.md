# EI_ESP32_CAM_SERVER

| Repo | CI status |
|------|-----------|
| CI on [dattazigzag repo](https://github.com/dattazigzag/EI_ESP32_CAM_SERVER) | [![Arduino CI](https://github.com/dattazigzag/EI_ESP32_CAM_SERVER/actions/workflows/arduino-ci.yml/badge.svg)](https://github.com/dattazigzag/EI_ESP32_CAM_SERVER/actions/workflows/arduino-ci.yml) |
| CI on [dattasaurabh82 repo](https://github.com/dattasaurabh82/EI_ESP32_CAM_SERVER) | [![Arduino CI](https://github.com/dattasaurabh82/EI_ESP32_CAM_SERVER/actions/workflows/arduino-ci.yml/badge.svg)](https://github.com/dattasaurabh82/EI_ESP32_CAM_SERVER/actions/workflows/arduino-ci.yml) |

---

## What is this?

<table>
  <tr>
    <td width="50%"><img src="assets/Screenshot 2025-02-11 at 02.08.08.png" width="100%" alt="First image"></td>
    <td width="50%"><img src="assets/PXL_20250224_233042468.PORTRAIT.jpg" width="100%" alt="Second image"></td>
  </tr>
  <tr>
    <td width="50%" align="center"><em>ESP self-hosted web UI</em></td>
    <td width="50%" align="center"><em>Xiao esp32-s3 with fan contraption</em></td>
  </tr>
</table>

This educational tool helps reduce the time needed for capturing and labeling ESP32 camera images for TinyML training in [edgeimpulse](https://docs.edgeimpulse.com/reference). It's more efficient than the standard method of capturing and uploading single images through the [edgeimpulse data forwarder firmware](https://github.com/edgeimpulse/firmware-espressif-esp32).

Instead, it creates an MJPEG stream directly from the camera and displays it on a camera-hosted frontend. This allows you to capture, label, and bulk upload images to the edgeimpulse studio—making the process more efficient than sitting there for a long time doing it manually.

## What triggered this development?

_In a nutshell_

1. The method to upload images via [edge-impulse-data-forwarder](https://docs.edgeimpulse.com/docs/tools/edge-impulse-cli/cli-data-forwarder) requires an intermediary computer. [While collecting and sending accelerometer and audio data is straightforward](https://docs.edgeimpulse.com/docs/tools/edge-impulse-cli/cli-data-forwarder), there's no simple example showing how to convert image data into a byte stream array. Though it's possible to do this manually (by capturing an image and converting it into a serializable byte stream), this functionality isn't readily available and requires significant programming effort, depending on your embedded systems expertise.
2. The Edge Impulse [data uploader firmware's](https://docs.edgeimpulse.com/docs/edge-ai-hardware/mcu/espressif-esp32) image upload method uses [WebSerial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) and can be seen directly in the studio. But the process to capture each single image is slow. While it offers a good browser-based experience and eliminates the need for a data forwarding middleware, [compiling the latest firmware (for custom board other than "ESP-EYE")](https://github.com/edgeimpulse/firmware-espressif-esp32) remains challenging for beginners.
3. [EloquentEsp32cam](https://eloquentarduino.com/posts/esp32-cam-object-detection) is nice and closer to what I was looking for as it hosts the web ui in packed binary format so everything is in c, finally and "one upload" functions flashes everything. But the trade off is that the Web UI modification is not straight forward (since it is gzipped) and the image transfer is also a multi step process requiring one to download labelled images first and then upload to edge impulse manually.

   | ![EloquentEsp32cam Web UI](assets/EloqUI.png) | ![Manual Upload Process](assets/manual_upload_process.png) |
   |:-------------------------------------------:|:----------------------------------------------------------:|
   | *EloquentEsp32cam Web UI for capture and download. <br>Courtesy: Author* | *Manual uploading process to Edge Impulse studio<br>Courtesy: [Marcelo Rovai](https://gravatar.com/mjrovai)* |

## What this project overcomes

### Advantages over other solution for image data capture

_In a nutshell_

1. WiFi configuration is flexible - users can set it up after compiling since the code logic accommodates this.

![alt text](<assets/Slide 16_9 - 2.png>)

<p align="center">
  <img src="assets/ScreenRecording2025-02-25at22.24.14-ezgif.com-video-to-gif-converter.gif" width="75%" alt="Image description"><br>
  <em>Configuring wifi from the Web UI</em>
</p>

> It will automatically show up, i.e. the ESP will go to AP mode and provide this captive portal like experience, if it fails to connect to any wifi from it's memory (or if the wifi memory doesn't exist yet)

<br> <br>
<img src="assets/Screenshot 2025-02-25 at 22.26.18.png" width="35%" alt="Image description">

> Once the esp32 connects, you will see this in Serial as you get re-directed to the main dashboard.

2. Multiple WiFi networks can be stored and persist between reboots, allowing seamless use across different locations without reconfiguring.
3. Stream can be started, stopped, and snapshots can be captured even while streaming is paused.
4. Supports both single snapshots and automatic multiple image capture.

<p align="center">
  <img src="assets/Screen Recording 2025-02-25 at 22.57.31.gif" width="75%" alt="Image description"><br>
  <em>Start/Stop stream and various capture modes demo</em>
</p>

5. Images can be labeled and downloaded in groups for manual upload to Edge Impulse, if preferred.
6. Uploads directly to Edge Impulse through the Web UI (the core purpose of this project). Edge Impulse API credentials persist between reboots.
7. Automatically updates Edge Impulse project settings for object detection if needed.

![alt text](assets/ScreenRecording2025-02-25at23.09.52.gif)

1. Images are stored in the browser session rather than Flash or RAM.
2. Features an attractive UI with both light and dark modes.
3. UI is easily modifiable through HTML and CSS updates, though re-flashing is required. See instructions below.

---

<details>
   <summary> 1. Hardware</summary>
  
   ## Hardware Setup
  
   Tested on: [XIAO_ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

   <br>

> The XIAO_ESP32S3 gets very hot when streaming MJPEG as stated [here](https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/#project-ii-video-streaming)

![alt text](<assets/Screenshot 2025-02-18 at 23.54.35.png>)

So I added a beefy cooper heat sink used in raspberry PIs and not the cheap aluminium ones and thought maybe I should just give it some air 💨

![alt text](<assets/Screenshot 2025-02-18 at 23.56.44.png>)

**But then how do I put it in as I like things to be organized and in place?**

So, I designed a cooling contraption for better air flow ...
And, additionally it holds everything together and also has a modular gorilla arm screw adapter.

| **Before** turning **ON** the fans | **After** turning **ON** the fans |
|:----------------------------------:|:---------------------------------:|
| ![Before turning on fans](<assets/Screenshot 2025-02-19 at 00.06.46.png>) | ![After turning on fans](<assets/Screenshot 2025-02-19 at 00.07.25.png>) |

Two points to note here:

1.  The OV5640 camera also gets 🥵.

   > !! Plan to fix that in next iteration

   <img src="assets/Screenshot 2025-02-19 at 00.13.18.png" width="50%" alt="Image description">

    

2.  The fan power is not drawn form the same VBUS that powers the XIAO_ESP32S3 but has a separate source, so that the performance of XIAO_ESP32S3 is not affected.

    > Yes! That means you need a separate cable if you do not want to fry your XIAO_ESP32S3.

   <img src="assets/Screenshot 2025-02-19 at 00.19.43.png" width="80%" alt="Image description">

    > My quick & dirty elegant solution

---

### xiao with cooling contraption and gorilla pod mount

![alt text](assets/xiao_with_cooling_contraption_and_gorilla_po_mount_render.png)

> Fusion 360 preview and file Download link: 👉🏼 [🌐](https://a360.co/3EEMBdH)

</details>

---

<details>
   <summary> 2. Edge Impulse Studio Project setup</summary>

   <br>

1.  Create an edge Impulse Project for `Object Detection`
2.  Give it a suitable name

    ![alt text](<assets/Screen Recording 2025-02-25 at 15.15.50.gif>)

3.  Note the Project ID and keep it safe somewhere. We will need that later to automatically upload images from the xiao esp32S3

    ![alt text](<assets/Screen Recording 2025-02-25 at 15.16.07.gif>)

4.  Note the Project's API Key. We will need that later to automatically upload images from the xiao esp32S3

    ![alt text](<assets/Screen Recording 2025-02-25 at 15.16.53.gif>)

</details>

---

<details>
   <summary> 3 . Software</summary>

# The Easy way

🤔 Since this project aims to simplify and speed up image data collection for Edge Impulse, I thought it would be better if users didn't need to set up a development environment at this early stage, to flash firmware.

The goal is to eliminate friction by removing the need for any development environment setup—even for simple tasks like configuring WiFi settings 😁

> After completing the machine learning training in Edge Impulse, you will need to download and use the model/library according to your own context and then you have to program...
<<<<<<< HEAD
> 
=======

>>>>>>> 9793cdf0887b01a218e0a19dcc2ca89e4dd5190a
So, I created a [web-based flasher tool](webflasher) (hosted on both [zigzag repo](https://dattazigzag.github.io/EI_ESP32_CAM_SERVER/) and [my personal repo](https://dattasaurabh82.github.io/EI_ESP32_CAM_SERVER/)) as part of the project. A website with all the necessary binary files and correct flashing settings, allowing you to connect your XIAO ESP32-S3 and flash everything directly from your browser—no Arduino IDE / Terminal or Platform IO setup needed! 😘

![alt text](assets/webflashing.gif)

> **Notes**:
>
> 1.  Although if you want to know how it all works, follow the ... [Arduino IDE compile and upload method](#arduino-ide-compile-and-upload-method) and or [cmdline compile and upload methods](#cmdline-compile-and-upload-methods)
>
> 2.  Post flashing, you can also setup Wifi Credentials (Persistent across boots)
>
> 3.  Two Github Action CI/CD pipelines accomplish them. You can learn more about them [here](.github/workflows), if you are keen on the Github Actions Pipeline that compiles and create releases of binaries and also updates the webflasher.

# Arduino IDE compile and upload method

Arduino IDE version: `2.3.4`

### Install libraries

1. [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)
2. [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)

> You can find them from the Library Manager of IDE. There are various versions. Install ones by "ESP32Async" for both the libraries.

### File upload - for frontend

We need to upload our files (html, css. js, etc. for the frontend) to esp-32 via [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)

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

3. Quit & reopen Arduino IDE. **Note:** Sometimes you might have to restart the mac
4. Pressing `CMD` + `SHIFT` + `P`, will open commands palette of Arduino IDE
5. Type in `Upload LittleFS` and the full command (`Upload LittleFS to Pico/ESP8266/ESP32`) will show up. Hit `ENTER`
6. All the contents from [`data/`](data/) will now be transferred to the fs of ESP32
   > Make sure Serial Monitor is closed

### Camera Settings

Pick a esp32 camera module based on the [camera_pins.h](camera_pins.h) and use only one in [camera_init.h](camera_init.h)

```c++
// Define camera model before including camera_pins.h
// e.g.: We are using XIAO_ESP32S3

// #define CAMERA_MODEL_AI_THINKER 1
#define CAMERA_MODEL_XIAO_ESP32S3 1
```

Most of the camera settings doesn't need to be changed but sometimes you may need to flip the camera frame vertically or horizontally. In that case [camera_init.h](camera_init.h) find the section

```c++
 // Additional camera settings after initialization
 sensor_t * s = esp_camera_sensor_get();
 if (s) {
     // Set frame size to desired resolution
     s->set_framesize(s, FRAMESIZE_QQVGA);  // 160x120
     // Flip camera vertically
     s->set_vflip(s, 1);
     // Flip camera horizontally
     // s->set_hmirror(s, 1)
 }
```

> More info here: [esp32-cam-ov2640-camera-settings](https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/)

### Server Port Settings

Our default web server is on port `80` defined in `WebServer server(80);` in our [EI_ESP32_CAM_SERVER.ino](EI_ESP32_CAM_SERVER.ino)

### Usage

After successful upload, you should see something like this

```txt

___ ESP32-CAM-WEB-SERVER - (edgeImpulse tool)___

1. Checking Camera Status:
   Initializing camera...
    [camera_init.h] PSRAM found ...
✓ Success

   Camera Details:
   --------------
   Resolution: 1x1
   Quality: 30
   Brightness: 0
   Contrast: 0
   Saturation: 0
   Special Effect: 0
   Vertical Flip: Yes
   Horizontal Mirror: No

   Memory Info:
   -----------
   PSRAM: Available ✓
   Free PSRAM: 8381488 bytes
   Total PSRAM: 8388608 bytes


2. Checking LittleFS Status:
   Mounting LittleFS... ✓ Mounted successfully (No formatting needed)

   Storage Info:
   ------------
   Total space: 1536 KB
   Used space: 84 KB
   Free space: 1452 KB

   Files in storage:
   ---------------
   • ei_config.json            157 bytes
   • ei_config.template.json       63 bytes
   • index.html               7400 bytes
   • script.js               23188 bytes
   • styles.css               8816 bytes
   • wifi_portal.css          5264 bytes
   • wifi_portal.html         2934 bytes
   • wifi_portal.js          14857 bytes


3. WiFi Manager Initialization:

3. WiFi Manager Initialization:
   ⚠ No WiFi credentials file found
   ⚠ No saved WiFi networks found
   Starting AP Mode for configuration
   ✓ AP started with SSID: XIAO_ESP32_CAM
   ✓ IP Address: 192.168.4.1
Async HTTP server started on port 80
```

# cmdline compile and upload methods

Let's say you just want to edit some basic html features and do not want to change any firmware settings and as a result do not want to go through the whole arduino IDE setup.

Even though that is a fairly straight forward route, for some reason you like being in terminal and want to do everything from there.

If that is the case, below are your compilation and update options.

1. Make sure to install `esptools.py`
   1. Information source 1: [here](https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html)
   2. Information source 2: [here](https://docs.espressif.com/projects/esptool/en/latest/esp32/index.html#quick-start)
   3. Information source 3: [here](https://tasmota.github.io/docs/Esptool/)
2. Make sure to install `arduino-cli`. Instructions 👉🏼 [here](https://github.com/arduino/arduino-cli)
3. After `arduino-cli` has been installed, install esp32 core, and library dependencies.

   ```bash
   # install esp32 core and boards
   arduino-cli config init
   arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   arduino-cli core update-index
   arduino-cli core install esp32:esp32

   # Install lib deps
   arduino-cli core update-index
   arduino-cli lib install ArduinoJson
   mkdir -p "$HOME/Arduino/libraries"
   cd "$HOME/Arduino/libraries"
   git clone https://github.com/ESP32Async/AsyncTCP.git
   git clone https://github.com/ESP32Async/ESPAsyncWebServer.git
   arduino-cli core update-index
   ```

4. Install `mklittlefs`. This is used to produce a packed binary of all the front-end files that can be flashed later. __Note__: if using the Arduino IDE, then we use a IDE plugin. Check it our 👉🏼 [file upload instructions](#file-upload---for-frontend) for more details.
   <br><br>

   > Prerequisite for this step: Make sure you have cmake, build essentials etc. ready and configured.
   >
   > 🔔 Don't worry as if and when the build command for `mklittlefs` fails, you will know what to install.
   > <br><br>

   ```bash
   git clone --recursive https://github.com/earlephilhower/mklittlefs.git
   cd mklittlefs
   make dist
   sudo cp mklittlefs /usr/local/bin/

   # source your env if needed

   mklittlefs --help
   ```

5. Create an empty `ei_config.json`. It will be filled with your credentials and edgeimpulse project details later, from frontend and will be saved to be used persistently till next update.

   ```bash
   cp data/ei_config.template.json data/ei_config.json
   ```

   Your data folder should now have these files

   ```txt
   data/
   ├── ei_config.json
   ├── ei_config.template.json
   ├── index.html
   ├── script.js
   ├── styles.css
   ├── wifi_portal.css
   ├── wifi_portal.html
   └── wifi_portal.js
   ```

6. Create a packed binary of all the front-end files of `data/`

   ```bash
   mkdir -p build

   # Create
   mklittlefs -c data -p 256 -b 4096 -s 1572864 build/filesystem.littlefs.bin

   # Verify
   mklittlefs -l -d 5 build/filesystem.littlefs.bin
   ```

7. Compile the firmware

   ```bash
   arduino-cli compile \
   --fqbn "esp32:esp32:XIAO_ESP32S3:USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=none,UploadSpeed=921600,JTAGAdapter=default" \
   --output-dir build . -v
   ```

8. Upload the firmware and packed frontend binaries (multiple options)

   ```bash
   # Option 1.1: Using arduino-cli - Compile & write the compiled firmware to target
   arduino-cli compile \
   --fqbn "esp32:esp32:XIAO_ESP32S3:USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,UploadSpeed=921600,JTAGAdapter=default" \
   . -u -p [YOUR_SERIAL_PORT_TO_WHICH_ESP32_IS_ATTACHED] -v

   # Option 1.2: Using arduino-cli - Write the pre-compiled firmware to target
   arduino-cli upload -p [YOUR_SERIAL_PORT_TO_WHICH_ESP32_IS_ATTACHED] \
   --fqbn "esp32:esp32:XIAO_ESP32S3:USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,UploadSpeed=921600,JTAGAdapter=default" \
   --input-file build/EI_ESP32_CAM_SERVER.ino.merged.bin .

   # Using esptools.py - Write ONLY the pre-compiled firmware to target
   esptool.py \
   --chip esp32s3 \
   --port [YOUR_SERIAL_PORT_TO_WHICH_ESP32_IS_ATTACHED] \
   --baud 921600 \
   --before default_reset \
   --after hard_reset write_flash \
   -z --flash_mode qio --flash_freq 80m --flash_size 8MB \
   0x0 build/EI_ESP32_CAM_SERVER.ino.merged.bin

   # Using esptools.py - Write the packed frontend binary to the target's correct location
   esptool.py \
   --chip esp32s3 \
   --port [YOUR_SERIAL_PORT_TO_WHICH_ESP32_IS_ATTACHED] \
   --baud 921600 write_flash -z \
   --flash_mode dio \
   --flash_freq 80m 0x670000 \
   build/filesystem.littlefs.bin
   ```

> Notes
>
> 1. `--flash_mode` is `qio` for flashing firmware and `--flash_mode` is `dio` for flashing packed frontend binary
>
> 2. And, how do we know the **exact location** in flash (`0x670000`) where the front end code goes?
> Well, we know it from the Arduino IDE. When we used the IDE plugin, we saw the output ...
>
> ![alt text](<assets/Screenshot 2025-02-25 at 14.17.52.png>)

</details>

---

## Acknowledgement & Attribution

```txt
Saurabh Datta
zigzag.is
Feb 2025
datta@zigzag.is
hi@dattasaurabh.com
```

---
