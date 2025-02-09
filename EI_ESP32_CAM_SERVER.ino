#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "credentials.h"
#include "camera_init.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


AsyncWebServer server(80);  // Single server instance

// ======== Non-blocking MJPEG Stream ========

/* Slower and safer */
// void handleMjpeg(AsyncWebServerRequest *request) {
//   AsyncWebServerResponse *response = request->beginChunkedResponse(
//     "multipart/x-mixed-replace; boundary=frame",
//     [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
//       camera_fb_t *fb = esp_camera_fb_get();
//       if (!fb) return 0;

//       // Assume maxLen >= 100 (header) + fb->len
//       size_t jpgLen = snprintf(
//         (char *)buffer, 100, // Limit header to first 100 bytes
//         "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",
//         fb->len
//       );

//       memcpy(buffer + jpgLen, fb->buf, fb->len);
//       esp_camera_fb_return(fb);

//       return jpgLen + fb->len;
//     }
//   );
//   response->addHeader("Access-Control-Allow-Origin", "*");
//   request->send(response);
// }

/* faster and less safer */
void handleMjpeg(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse(
    "multipart/x-mixed-replace; boundary=frame",
    [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) return 0;

      size_t jpgLen = snprintf(
        (char *)buffer, maxLen,
        "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",
        fb->len);
      memcpy(buffer + jpgLen, fb->buf, fb->len);
      esp_camera_fb_return(fb);

      return jpgLen + fb->len;
    });
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}


// Image saving
void handleCapture(AsyncWebServerRequest *request) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    request->send(500, "text/plain", "Camera capture failed");
    return;
  }
  request->send(200, "image/jpeg", fb->buf, fb->len);
  esp_camera_fb_return(fb);
}


// Camera init with verbose output
void initCamera() {
  Serial.println("\n1. Checking Camera Status:");
  Serial.print("   Initializing camera... ");

  if (setupCamera()) {
    Serial.println("✓ Success");

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
      Serial.println("\n   Camera Details:");
      Serial.println("   --------------");
      Serial.printf("   Resolution: %dx%d\n", sensor->status.framesize, sensor->status.framesize);
      Serial.printf("   Quality: %d\n", sensor->status.quality);
      Serial.printf("   Brightness: %d\n", sensor->status.brightness);
      Serial.printf("   Contrast: %d\n", sensor->status.contrast);
      Serial.printf("   Saturation: %d\n", sensor->status.saturation);
      Serial.printf("   Special Effect: %d\n", sensor->status.special_effect);
      Serial.printf("   Vertical Flip: %s\n", sensor->status.vflip ? "Yes" : "No");
      Serial.printf("   Horizontal Mirror: %s\n", sensor->status.hmirror ? "Yes" : "No");
    }
    if (psramFound()) {
      Serial.println("\n   Memory Info:");
      Serial.println("   -----------");
      Serial.println("   PSRAM: Available ✓");
      Serial.printf("   Free PSRAM: %lu bytes\n", ESP.getFreePsram());
      Serial.printf("   Total PSRAM: %lu bytes\n", ESP.getPsramSize());
    } else {
      Serial.println("\n   ⚠ WARNING: No PSRAM detected");
      Serial.println("   Camera will operate with limited buffer size");
    }
  } else {
    Serial.println("✗ Failed");
    Serial.println("   ❌ Fatal Error: Camera initialization failed");
    Serial.println("   Please check camera connection and pins");
    return;
  }

  Serial.println();
}


void initLittleFS() {
  Serial.println("\n2. Checking LittleFS Status:");
  Serial.print("   Mounting LittleFS... ");

  if (LittleFS.begin(false)) {
    Serial.println("✓ Mounted successfully (No formatting needed)");
  } else {
    Serial.println("✗ Mount failed");
    Serial.print("   Attempting to format... ");

    if (LittleFS.format()) {
      Serial.println("✓ Format successful");
      Serial.print("   Trying to mount again... ");

      if (LittleFS.begin()) {
        Serial.println("✓ Mounted successfully");
        Serial.println("   ⚠ WARNING: File system is empty!");
        Serial.println("   ⚠ Please upload files using ESP32 LittleFS Data Upload");
        Serial.println("   ⚠ Do not forget to close the serail monitor before");
        Serial.println("   ⚠ Then reset the device.");
      } else {
        Serial.println("✗ Mount failed after format");
        Serial.println("   ❌ Fatal Error: Storage unavailable");
        return;
      }
    } else {
      Serial.println("✗ Format failed");
      Serial.println("   ❌ Fatal Error: Unable to initialize storage");
      return;
    }
  }
  // Print LittleFS info for ESP32
  Serial.println("\n   Storage Info:");
  Serial.println("   ------------");
  Serial.printf("   Total space: %u KB\n", LittleFS.totalBytes() / 1024);
  Serial.printf("   Used space: %u KB\n", LittleFS.usedBytes() / 1024);
  Serial.printf("   Free space: %u KB\n", (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024);

  // List all files
  Serial.println("\n   Files in storage:");
  Serial.println("   ---------------");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("   • %-20s %8u bytes\n", fileName.c_str(), fileSize);
    file = root.openNextFile();
  }

  Serial.println();
}


void setupWIFIstn() {
  Serial.println("\n3. Checking WiFi Status:");
  Serial.printf("   Connecting to SSID: %s ", ssid);

  uint8_t attempts = 0;
  const uint8_t max_attempts = 20;  // 10 seconds maximum

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✓ Connected!");

    Serial.println("\n   Network Info:");
    Serial.println("   ------------");
    Serial.printf("   ⤷ IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("   ⤷ Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("   ⤷ Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("   ⤷ DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("   ⤷ MAC Address: %s\n", WiFi.macAddress().c_str());

    Serial.println("\n   Signal Info:");
    Serial.println("   -----------");
    Serial.printf("   ⤷ RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("   ⤷ Channel: %ld\n", WiFi.channel());
    Serial.printf("   ⤷ TX Power: %d dBm\n", WiFi.getTxPower());

    Serial.println("\n   Connection Info:");
    Serial.println("   ---------------");
    Serial.printf("   ⤷ SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("   ⤷ Connection Time: %u ms\n", attempts * 500);
  } else {
    Serial.println(" ✗ Failed!");
    Serial.println("   ❌ Fatal Error: WiFi connection failed");
    Serial.println("   Please check credentials and router status");
    return;
  }

  Serial.println();
}


void setup() {
  // Brownout prevention
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Configure PSRAM cache strategy
  psramInit();
  heap_caps_malloc_extmem_enable(512);  // Allocate PSRAM first

  // Camera power pin stabilization (AI Thinker specific)
  pinMode(12, OUTPUT);  // ESP32-CAM Flash LED pin
  digitalWrite(12, LOW);

  Serial.begin(115200);
  delay(3000);
  Serial.println();
  Serial.println("___ ESP32-CAM-WEB-SERVER - (edgeImpulse tool)___");

  // 1. Cam init
  initCamera();
  // 2. LittleFS init
  initLittleFS();
  // 3. Connect to WiFi
  setupWIFIstn();

  // 4. Configure AsyncWebServer Routes
  // Static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "application/javascript");
  });

  // Stream endpoint
  server.on("/stream", HTTP_GET, handleMjpeg);

  // Capture endpoint
  server.on("/capture", HTTP_GET, handleCapture);

  // Clear endpoint
  server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Images cleared");
  });

  server.begin();
  Serial.println("Async HTTP server started on port 80");
}

void loop() {
  delay(1);
}