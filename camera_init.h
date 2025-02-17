#include "sensor.h"
#ifndef CAMERA_INIT_H
#define CAMERA_INIT_H

// Define camera model before including camera_pins.h
// #define CAMERA_MODEL_AI_THINKER 1
#define CAMERA_MODEL_XIAO_ESP32S3 1

#include "esp_camera.h"
#include "camera_pins.h"

bool setupCamera();

bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Initial buffer size settings
  if (psramFound()) {
    Serial.println();
    Serial.println("    [camera_init.h] PSRAM found ...");

    config.frame_size = FRAMESIZE_QQVGA;  // 160x120
    config.jpeg_quality = 30;             // 0-63: lower means higher quality
    config.fb_count = 1;
  } else {
    Serial.println();
    Serial.println("    [camera_init.h] PSRAM Not found ...");

    config.frame_size = FRAMESIZE_QQVGA;  // Still 160x120
    config.jpeg_quality = 40;             // 0-63: lower means higher quality
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[camera_init.h] Camera init failed with error 0x%x", err);
    return false;
  }

  // Additional camera settings after initialization
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    // Set frame size to desired resolution
    s->set_framesize(s, FRAMESIZE_QQVGA);  // 160x120
    // Flip camera vertically
    s->set_vflip(s, 1);
    // Flip camera horizontally
    // s->set_hmirror(s, 1)
  }

  return true;
}

#endif /* CAMERA_INIT_H */