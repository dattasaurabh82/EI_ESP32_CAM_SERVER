// #ifndef SERVER_HANDLERS_H
// #define SERVER_HANDLERS_H

// #include "esp_http_server.h"
// #include "esp_camera.h"

// extern httpd_handle_t webServer;

// // Stream defines
// #define PART_BOUNDARY "123456789000000000000987654321"
// static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
// static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
// static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// void startCameraServer();

// #endif

#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "esp_http_server.h"
#include "esp_camera.h"

// Only define the boundary string in header as it's used in multiple places
#define PART_BOUNDARY "123456789000000000000987654321"

extern httpd_handle_t webServer;
void startCameraServer();

#endif