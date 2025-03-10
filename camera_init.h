#ifndef CAMERA_INIT_H
#define CAMERA_INIT_H

// Include the configuration file first
#include "config.h"

#include "sensor.h"
#include "esp_camera.h"
#include "camera_pins.h"

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

  // Model-specific settings
#ifdef CAMERA_MODEL_XIAO_ESP32S3
  Serial.println("\t[camera_init.h] Using XIAO ESP32S3 camera settings");
#elif defined(CAMERA_MODEL_AI_THINKER)
  Serial.println("\t[camera_init.h] Using AI-THINKER ESP32-CAM settings");
#elif defined(CAMERA_MODEL_ESP_EYE)
  Serial.println("\t[camera_init.h] Using ESP-EYE V2 camera settings");
#endif

  // Buffer configuration based on PSRAM availability
  if (psramFound()) {
    Serial.println();
    Serial.println("\t[camera_init.h] PSRAM found ...");
    Serial.printf("\t[camera_init.h] PSRAM enabled: %s\n", psramFound() ? "YES" : "NO");
    Serial.printf("\t[camera_init.h] Free PSRAM: %lu bytes\n", ESP.getFreePsram());

    // config.frame_size = FRAMESIZE_QQVGA;  // 160x120
    config.frame_size = FRAMESIZE_QVGA;  // 320x240 (higher quality)

#ifdef CAMERA_MODEL_XIAO_ESP32S3
    config.jpeg_quality = 24;  // 0-63: lower means higher quality
    config.fb_count = 2;       // Double buffering for smoother
#elif defined(CAMERA_MODEL_AI_THINKER)
    config.jpeg_quality = 30;  // 0-63: lower means higher quality
    config.fb_count = 1;       // Single buffering for less mem overload
#elif defined(CAMERA_MODEL_ESP_EYE)
    config.jpeg_quality = 10;  // 0-63: lower means higher quality
    config.fb_count = 2;       // Double buffering for smoother
#endif
  } else {
#ifdef CAMERA_MODEL_XIAO_ESP32S3
    config.jpeg_quality = 35;  // 0-63: lower means higher quality
    config.fb_count = 1;       // Single buffering for less mem overload
#elif defined(CAMERA_MODEL_AI_THINKER)
    config.jpeg_quality = 40;  // 0-63: lower means higher quality
    config.fb_count = 1;       // Single buffering for less mem overload
#elif defined(CAMERA_MODEL_ESP_EYE)
    config.jpeg_quality = 35;  // 0-63: lower means higher quality
    config.fb_count = 1;       // Single buffering for less mem overload
#endif
  }

  Serial.printf("\t[camera_init.h] Frame buffer count set to: %d\n", config.fb_count);

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[camera_init.h] Camera init failed with error 0x%x", err);
    return false;
  }

  // Additional camera settings after initialization
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_QQVGA);  // 160x120
    s->set_framesize(s, FRAMESIZE_QVGA);   // 320x240 (higher quality)

    // Model-specific Initial camera orientation settings
#ifdef CAMERA_MODEL_XIAO_ESP32S3
    s->set_vflip(s, 1);    // Flip camera vertically for XIAO
    s->set_hmirror(s, 0);  // No horizontal mirror for XIAO
#elif defined(CAMERA_MODEL_AI_THINKER)
    s->set_vflip(s, 1);        // Flip camera vertically for AI-Thinker
    s->set_hmirror(s, 1);      // Horizontal mirror typically needed
#elif defined(CAMERA_MODEL_ESP_EYE)
    s->set_vflip(s, 0);        // No Flipping of camera vertically
    s->set_hmirror(s, 0);      // No Horizontal mirror
#endif

    // Image clarity enhancements
    s->set_brightness(s, 2);  // Normal brightness (-2 to 2)
    s->set_contrast(s, 1);    // Normal contrast (-2 to 2)
    s->set_saturation(s, 1);  // Normal saturation (-2 to 2)

    // --- //
    /*
     * NOTE [TBT]
     * White balance implementation varies by camera sensor. The XIAO ESP32S3 * * uses an OV sensor that might handle white balance differently than the * * ESP32 camera library expects. And so, the status.wb_mode field sometimes * doesn't accurately reflect the actual * camera state.
    */
#ifdef CAMERA_MODEL_XIAO_ESP32S3
    s->set_whitebal(s, 1);  // Disable white balance (0=disable, 1=enable)
    s->set_awb_gain(s, 1);  // Disable auto white balance gain (0=disable,
#elif defined(CAMERA_MODEL_AI_THINKER)
    // Note: Some color correction needed (noticed)
    s->set_whitebal(s, 1);  // Disable white balance (0=disable, 1=enable)
    s->set_awb_gain(s, 1);  // Disable auto white balance gain (0=disable,
#elif defined(CAMERA_MODEL_ESP_EYE)
    s->set_whitebal(s, 1);     // Enable white balance
    s->set_awb_gain(s, 1);     // Enable auto white balance gain
#endif

    // --- //

    s->set_gainceiling(s, GAINCEILING_2X);  // Normal gain
  }
  return true;
}

#endif /* CAMERA_INIT_H */