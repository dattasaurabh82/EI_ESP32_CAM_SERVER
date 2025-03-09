#ifndef CONFIG_H
#define CONFIG_H

// -----------------------------------------------------------------
// CAMERA MODEL SELECTION - UNCOMMENT ONLY ONE MODEL
// -----------------------------------------------------------------

// #define CAMERA_MODEL_XIAO_ESP32S3 1
// #define CAMERA_MODEL_AI_THINKER 1
#define CAMERA_MODEL_ESP_EYE 1

// Error checking for camera model selection
#if defined(CAMERA_MODEL_XIAO_ESP32S3) && defined(CAMERA_MODEL_AI_THINKER) && defined(CAMERA_MODEL_ESP_EYE)
#error "Please select only one camera model by uncommenting its define"
#endif

#if defined(CAMERA_MODEL_XIAO_ESP32S3) && defined(CAMERA_MODEL_AI_THINKER) && !defined(CAMERA_MODEL_ESP_EYE)
#error "Please select only one camera model by uncommenting its define"
#endif

#if defined(CAMERA_MODEL_XIAO_ESP32S3) && !defined(CAMERA_MODEL_AI_THINKER) && defined(CAMERA_MODEL_ESP_EYE)
#error "Please select only one camera model by uncommenting its define"
#endif

#if !defined(CAMERA_MODEL_XIAO_ESP32S3) && defined(CAMERA_MODEL_AI_THINKER) && defined(CAMERA_MODEL_ESP_EYE)
#error "Please select only one camera model by uncommenting its define"
#endif

#if !defined(CAMERA_MODEL_XIAO_ESP32S3) && !defined(CAMERA_MODEL_AI_THINKER) && !defined(CAMERA_MODEL_ESP_EYE)
#error "Please select a camera model by uncommenting its define"
#endif

// -----------------------------------------------------------------
// OTHER CONFIGURATION PARAMETERS
// -----------------------------------------------------------------
// Add any other global configuration parameters will go here
// 
// Note: nothing for now

#endif // CONFIG_H