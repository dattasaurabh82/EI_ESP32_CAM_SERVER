#include "config.h"  // Include first and foremost the camera model selection configuration

#ifdef CAMERA_MODEL_XIAO_ESP32S3
// No brownout includes needed
#elif defined(CAMERA_MODEL_AI_THINKER)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#elif defined(CAMERA_MODEL_ESP_EYE)
// ESP-EYE typically doesn't need brownout prevention
#endif

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define CONFIG_LITTLEFS_CACHE_SIZE 512

#include "wifi_manager.h"
#include "camera_init.h"
#include "gzipped_assets.h"      // Include the generated header with compressed frontend assets
#include "gzip_direct_routes.h"  // Include the GZIP serving handler



AsyncWebServer server(80);  // Single server instance
WifiManager wifiManager;    // Create WiFi manager instance

bool isStreamActive = true;  // Flag to track Streaming State status
bool isConnecting = false;   // Flag to track WiFi connection status

String lastConnectionSSID = "";
String lastConnectionPassword = "";

// FreeRTOS task Used for Resetting over serial
// ** (This happens completely independently of your main loop)
TaskHandle_t serialMonitorTaskHandle;




// ======== Non-blocking MJPEG Stream ========
void handleMjpeg(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse(
    "multipart/x-mixed-replace; boundary=frame",
    [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      if (!isStreamActive) {
        return 0;  // Return 0 to stop streaming
      }

      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) return 0;

      size_t jpgLen = snprintf((char *)buffer, 100,
                               "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",
                               fb->len);

      if (jpgLen + fb->len > maxLen) {
        esp_camera_fb_return(fb);
        return 0;
      }

      memcpy(buffer + jpgLen, fb->buf, fb->len);
      esp_camera_fb_return(fb);
      return jpgLen + fb->len;
    });
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}


// Image saving
void handleCapture(AsyncWebServerRequest *request) {
  Serial.println("Received Save frame request ...");
  static unsigned long lastCapture = 0;
  unsigned long now = millis();

  if (now - lastCapture < 1000) {
    request->send(429, "text/plain", "Too many requests");
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    request->send(503, "text/plain", "Camera busy");
    return;
  }

  request->send(200, "image/jpeg", fb->buf, fb->len);
  esp_camera_fb_return(fb);
  lastCapture = now;
}


// Camera init with verbose output
void initCamera() {
  // Extra stabilization delay
  delay(500);

  Serial.println("\n1. Checking Camera Status:");
  Serial.print("\t* Initializing ");

#ifdef CAMERA_MODEL_XIAO_ESP32S3
  Serial.print("XIAO ESP32S3 SENSE");
#elif defined(CAMERA_MODEL_AI_THINKER)
  Serial.print("AI-Thinker ESP32");
#endif
  Serial.println("camera... ");

  if (setupCamera()) {
    Serial.println("\t✓ Success");

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
      Serial.println("\n\tCamera Details:");
      Serial.println("\t--------------");
      Serial.printf("\tResolution: %dx%d\n", sensor->status.framesize, sensor->status.framesize);
      Serial.printf("\tJPEG Quality: %d\n", sensor->status.quality);

      // Get camera handle to access buffer information to print buffer information
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        Serial.printf("\tBuffer Size: %d bytes\n", fb->len);
        Serial.printf("\tBuffer Width: %d px\n", fb->width);
        Serial.printf("\tBuffer Height: %d px\n", fb->height);
        // Return the frame buffer so it can be reused
        esp_camera_fb_return(fb);
      }

      Serial.printf("\tBrightness: %d\n", sensor->status.brightness);
      Serial.printf("\tContrast: %d\n", sensor->status.contrast);
      Serial.printf("\tSaturation: %d\n", sensor->status.saturation);
      Serial.printf("\tSpecial Effect: %d\n", sensor->status.special_effect);
      Serial.printf("\tWhite Balance Enabled (human readable): %s\n", sensor->status.wb_mode ? "Yes" : "No");
      Serial.printf("\tAWB Gain Enabled: %s\n", sensor->status.awb_gain ? "Yes" : "No");
      Serial.printf("\tGain Ceiling Value: %d\n", sensor->status.gainceiling);  // Gain ceiling - this is an enum value (integer)
      // Translate gain ceiling to human-readable format
      const char *gainCeilingText;
      switch (sensor->status.gainceiling) {
        case GAINCEILING_2X: gainCeilingText = "2X"; break;
        case GAINCEILING_4X: gainCeilingText = "4X"; break;
        case GAINCEILING_8X: gainCeilingText = "8X"; break;
        case GAINCEILING_16X: gainCeilingText = "16X"; break;
        case GAINCEILING_32X: gainCeilingText = "32X"; break;
        case GAINCEILING_64X: gainCeilingText = "64X"; break;
        case GAINCEILING_128X: gainCeilingText = "128X"; break;
        default: gainCeilingText = "Unknown"; break;
      }
      Serial.printf("\tGain Ceiling: %s\n", gainCeilingText);

      Serial.printf("\tVertical Flip: %s\n", sensor->status.vflip ? "Yes" : "No");
      Serial.printf("\tHorizontal Mirror: %s\n", sensor->status.hmirror ? "Yes" : "No");
    }
    if (psramFound()) {
      Serial.println("\n\tMemory Info:");
      Serial.println("\t-----------");
      Serial.println("\tPSRAM: Available ✓");
      Serial.printf("\tFree PSRAM: %lu bytes\n", ESP.getFreePsram());
      Serial.printf("\tTotal PSRAM: %lu bytes\n", ESP.getPsramSize());
    } else {
      Serial.println("\n\t⚠ WARNING: No PSRAM detected");
      Serial.println("\tCamera will operate with limited buffer size");
    }
  } else {
    Serial.println("\t✗ Failed");
    Serial.println("\t❌ Fatal Error: Camera initialization failed");
    Serial.println("\tPlease check camera connection and pins");
    return;
  }

  Serial.println();
}


void initLittleFS() {
  Serial.println("\n2. Checking LittleFS Status:");
  Serial.println("\t* Mounting LittleFS... ");

  if (LittleFS.begin(false)) {
    Serial.println("\t✓ Mounted successfully");

    // Check for configuration files
    if (!LittleFS.exists("/ei_config.json")) {
      Serial.println("\t⚠ No Edge Impulse configuration file found");
      Serial.println("\t⚠ You would need to save one from the dashboard");
    }
    if (!LittleFS.exists("/wifi_credentials.json")) {
      Serial.println("\t⚠ No WiFi credentials file found");
      Serial.println("\t⚠ You would need to conect to one and it will be saved");
    }
    // Make sure LittleFS contains no old web files
    Serial.println("\n\tFiles in LittleFS:");
    Serial.println("\t---------------");
    int filesRemoved = 0;
    for (const char *oldFile : { "/index.html", "/styles.css", "/script.js",
                                 "/wifi_portal.html", "/wifi_portal.css", "/wifi_portal.js" }) {
      if (LittleFS.exists(oldFile)) {
        Serial.printf("\t- Removing old file: %s\n", oldFile);
        LittleFS.remove(oldFile);
        filesRemoved++;
      }
    }
    if (filesRemoved == 0) {
      Serial.println("\t✓ No old web files found.");
    } else {
      Serial.printf("\t✓ Cleaned %d old web files from filesystem\n", filesRemoved);
    }
    Serial.println("\t---------------");
    // List all files
    Serial.println("\n\tFiles in LittleFS:");
    Serial.println("\t---------------");
    File root = LittleFS.open("/");
    if (!root) {
      Serial.println("\t❌ Failed to open directory");
    } else if (!root.isDirectory()) {
      Serial.println("\t❌ Not a directory");
    } else {
      File file = root.openNextFile();
      if (!file) {
        Serial.println("\t(no files)");
      }
      int fileCount = 0;
      size_t totalSize = 0;

      while (file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        Serial.printf("\t• %-20s %8u bytes\n", fileName.c_str(), fileSize);
        totalSize += fileSize;
        fileCount++;
        file = root.openNextFile();
      }

      if (fileCount > 0) {
        Serial.printf("\n\tTotal: %d files, %u bytes\n", fileCount, totalSize);
      }
    }
  } else {
    Serial.println("\t✗ Mount failed");
    Serial.print("\tAttempting to format... ");

    if (LittleFS.format()) {
      Serial.println("\t✓ Format successful");
      Serial.print("\tTrying to mount again... ");

      if (LittleFS.begin()) {
        Serial.println("\t✓ Mounted successfully");
      } else {
        Serial.println("\t✗ Mount failed after format");
        Serial.println("\t❌ Fatal Error: Storage unavailable");
        return;
      }
    } else {
      Serial.println("\t✗ Format failed");
      Serial.println("\t❌ Fatal Error: Unable to initialize storage");
      return;
    }
  }

  // Print storage information
  Serial.println("\n\tStorage Info:");
  Serial.println("\t------------");
  Serial.printf("\tTotal space: %u KB\n", LittleFS.totalBytes() / 1024);
  Serial.printf("\tUsed space: %u KB\n", LittleFS.usedBytes() / 1024);
  Serial.printf("\tFree space: %u KB\n", (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024);
}


void setupWIFIstn() {
  // Initialize the WiFi manager
  wifiManager.begin();

  // Try to connect to saved networks first
  if (!wifiManager.connectToSavedNetworks()) {
    // If no saved networks or connection fails, start AP mode
    wifiManager.startAPMode();
  }
}


// Function to print memory stats [monitoring]
void printMemoryStats() {
  Serial.println("\n=== MEMORY STATS ===");
  Serial.printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %lu bytes\n", ESP.getFreePsram());
  Serial.printf("Minimum free heap: %lu bytes\n", ESP.getMinFreeHeap());
  Serial.println("===================\n");
}



void setup() {
  // Brownout handling ...
#ifdef CAMERA_MODEL_XIAO_ESP32S3
  // Skip brownout and pin settings
#elif defined(CAMERA_MODEL_AI_THINKER)
  // Brownout prevention
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // ** Note: Do not set Pin 4, 10 & 12 for OUTPUT/LOW
#elif defined(CAMERA_MODEL_ESP_EYE)
  // Skip brownout and pin settings
#endif

  delay(1000);
  Serial.begin(115200);

  // Some small delay to wait for serial to begin
  Serial.println("\nWaiting 5 secs ...\n");
  delay(5000);

  /*
  // ==== Debug Print GZIP Assets ==== //
  Serial.println("\n====== GZIP Assets Debug ======");
  Serial.printf("GZIP_ASSETS_COUNT: %d\n", GZIP_ASSETS_COUNT);
  if (GZIP_ASSETS_COUNT > 0) {
    Serial.println("Available assets:");
    for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
      Serial.printf("  %s (%s, %u bytes)\n",
                    GZIP_ASSETS[i].path,
                    GZIP_ASSETS[i].content_type,
                    GZIP_ASSETS[i].length);
    }
  } else {
    Serial.println("ERROR: No GZIP assets available!");
  }
  // Make sure LittleFS contains no old web files
  // ** Note: Moved into initLittleFS(){} 
  */

  // Model-specific welcome message
#ifdef CAMERA_MODEL_XIAO_ESP32S3
  Serial.println("\n___ XIAO ESP32S3 CAM-WEB-SERVER - (edgeImpulse tool)___");
#elif defined(CAMERA_MODEL_AI_THINKER)
  Serial.println("\n___ AI-THINKER ESP32-CAM-WEB-SERVER - (edgeImpulse tool)___");
#elif defined(CAMERA_MODEL_ESP_EYE)
  Serial.println("\n___ ESP-EYE V2 CAM-WEB-SERVER - (edgeImpulse tool)___");
#endif

// 0. Create a task dedicated to monitoring serial input
#ifdef CAMERA_MODEL_XIAO_ESP32S3
  xTaskCreate(
    serialMonitorTask,        // Function to implement the task
    "SerialMonitorTask",      // Name of the task
    2048,                     // Stack size in words
    NULL,                     // Task input parameter
    1,                        // Priority of the task
    &serialMonitorTaskHandle  // Task handle
  );
#elif defined(CAMERA_MODEL_AI_THINKER)
  // For AI-Thinker, create task on Core 0 with larger stack
  xTaskCreatePinnedToCore(
    serialMonitorTask,
    "SerialMonitorTask",
    4096,  // Double stack size
    NULL,
    1,  // Keep low priority
    &serialMonitorTaskHandle,
    0  // Run on Core 0 (Arduino loop uses Core 1)
  );
  // Add extra delay for AI-Thinker
  delay(1000);
#elif defined(CAMERA_MODEL_ESP_EYE)
  xTaskCreate(
    serialMonitorTask,
    "SerialMonitorTask",
    2048,  // Stack size (adjust if needed)
    NULL,
    1,  // Priority
    &serialMonitorTaskHandle);
#endif


  // 1. Cam init
  initCamera();
  delay(1000);

  // 2. LittleFS init
  initLittleFS();
  delay(1000);

  // 3. Connect to Wi Fi
  setupWIFIstn();

  // 4. gzip handler as the first handler for static files
  setupGzipRoutes(server);

  // 5. Stream endpoints
  server.on("/toggleStream", HTTP_POST, [](AsyncWebServerRequest *request) {
    isStreamActive = !isStreamActive;
    Serial.printf("\tStream state: %s\n", isStreamActive ? "Active" : "Paused");
    request->send(200, "text/plain", isStreamActive ? "streaming" : "paused");
  });

  server.on("/stream", HTTP_GET, handleMjpeg);

  // 6. Othr API end points
  // 6.1. Capture endpoint
  server.on("/capture", HTTP_GET, handleCapture);

  // 6.2. Clear endpoint: Both GET & POST
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("\tReceived Clear all GET request ...");
    request->send(200, "text/plain", "Images cleared (GET)");
  });
  server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("\tReceived Clear all POST request ...");
    request->send(200, "text/plain", "Images cleared (POST)");
  });

  // 6.3. Save config endpoint
  server.on("/saveConfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("config", true)) {
      String config = request->getParam("config", true)->value();
      File file = LittleFS.open("/ei_config.json", "w");
      if (file) {
        file.print(config);
        file.close();
        Serial.println("\tConfiguration saved to LittleFS ...");
        request->send(200, "text/plain", "Configuration saved");
      } else {
        Serial.println("\tFailed to save configuration to LittleFS ...");
        request->send(500, "text/plain", "Failed to save configuration");
      }
    } else {
      request->send(400, "text/plain", "No configuration data");
    }
  });

  // 6.4. Load config endpoint
  server.on("/loadConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/ei_config.json")) {
      request->send(LittleFS, "/ei_config.json", "application/json");
      Serial.println("  Configuration loaded from LittleFS ...");
    } else {
      Serial.println("\tNo configuration found in LittleFS ...");
      request->send(404, "text/plain", "No configuration found");
    }
  });

  // 7. WiFi Manager API endpoints
  server.on("/wifi/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["apMode"] = wifiManager.isAPMode();
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    wifiManager.scanNetworks();
    request->send(200, "text/plain", "Scan started");
  });

  server.on("/wifi/networks", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (wifiManager.isScanComplete()) {
      request->send(200, "application/json", wifiManager.getNetworkListJson());
    } else {
      request->send(202, "text/plain", "Scan in progress");
    }
  });

  server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      lastConnectionSSID = request->getParam("ssid", true)->value();
      lastConnectionPassword = request->getParam("password", true)->value();

      // Start connection process (non-blocking)
      isConnecting = true;
      // We need to respond to the client before attempting connection
      request->send(200, "text/plain", "Connection attempt started");
      // Connection attempt will be handled in the loop() function
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["connecting"] = isConnecting;
    if (WiFi.status() == WL_CONNECTED) {
      doc["ip"] = WiFi.localIP().toString();
      doc["ssid"] = WiFi.SSID();
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  server.on("/wifi/stopAP", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Respond first, then stop AP mode
    request->send(200, "text/plain", "Stopping AP mode");
    Serial.println("  Exiting AP MODE ...");
    // Schedule AP mode stop after response is sent
    WiFi.softAPdisconnect(true);
    wifiManager.setAPMode(false);
    //
    Serial.printf("👉🏼 Open http://%s:80 from a browser of a computer connected to WiFi SSID: %s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.SSID().c_str());
  });

  // Add a catch-all handler for debugging
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("❌ 404 Not Found: %s\n", request->url().c_str());
    request->send(404, "text/plain", "Not found - Path: " + request->url());
  });

  delay(1000);
  server.begin();

  Serial.println("Async HTTP server started on port 80\n");
  if (wifiManager.isAPMode()) {
    Serial.printf("👉🏼 Open http://%s:80 from a browser of a computer connected to WiFi SSID: %s\n",
                  AP_IP.toString().c_str(),
                  AP_SSID);
  } else {
    Serial.printf("👉🏼 Open http://%s:80 from a browser of a computer connected to WiFi SSID: %s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.SSID().c_str());
  }

  printMemoryStats();
}


void loop() {
  // Monitor HEAP and PSRAM USAGE and apply a more aggressive restart control
  /*
  1. AI-Thinker ESP32-CAM:
    - Higher PSRAM threshold (20KB vs 10KB) because it has half the PSRAM of XIAO
    - Higher heap threshold (25KB vs 20KB) because the original ESP32 can be less efficient with memory
    - Camera operations may consume memory differently on older ESP32
  2. XIAO ESP32S3:
    - Can operate with lower thresholds due to more available PSRAM
    - ESP32-S3 has improved memory management over the original ESP32
  */

  // Server Load/CPU Usage monitor
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > monitorMEMafter) {
    lastMemCheck = millis();
    printMemoryStats();
  }

#ifdef CAMERA_MODEL_XIAO_ESP32S3
  // Thresholds for XIAO with 8MB PSRAM
  if (ESP.getFreeHeap() < 20000 || ESP.getFreePsram() < 10000) {
    Serial.println("🏮 Hit memory utilization ceiling...");
    Serial.printf("\tFree PSRAM: %lu bytes\n", ESP.getFreePsram());
    Serial.printf("\tFree Heap: %lu bytes\n\n", ESP.getFreeHeap());
    Serial.println("\tLow memory: Restarting\n");
    ESP.restart();
  }
#elif defined(CAMERA_MODEL_AI_THINKER)
  // More conservative thresholds for AI-Thinker with 4MB PSRAM
  if (ESP.getFreeHeap() < 15000 || ESP.getFreePsram() < 10000) {
    Serial.println("🏮 Hit memory utilization ceiling...");
    Serial.printf("\tFree PSRAM: %lu bytes\n", ESP.getFreePsram());
    Serial.printf("\tFree Heap: %lu bytes\n\n", ESP.getFreeHeap());
    Serial.println("\tLow memory: Restarting\n");
    ESP.restart();
  }
#elif defined(CAMERA_MODEL_ESP_EYE)
  // ESP-EYE has 8MB PSRAM (same as XIAO)
  if (ESP.getFreeHeap() < 20000 || ESP.getFreePsram() < 10000) {
    Serial.println("🏮 Hit memory utilization ceiling...");
    Serial.printf("\tFree PSRAM: %lu bytes\n", ESP.getFreePsram());
    Serial.printf("\tFree Heap: %lu bytes\n\n", ESP.getFreeHeap());
    Serial.println("\tLow memory: Restarting\n");
    ESP.restart();
  }
#endif

  // Handle WiFi connection requests (non-blocking)
  if (isConnecting) {
    static String pendingSSID;
    static String pendingPassword;
    static unsigned long connectionStartTime = 0;
    static bool connectionInitiated = false;

    if (!connectionInitiated) {
      // Store the pending connection parameters
      pendingSSID = lastConnectionSSID;  // Use global variables set in the handler
      pendingPassword = lastConnectionPassword;
      connectionStartTime = millis();
      connectionInitiated = true;

      // Attempt connection
      bool result = wifiManager.connectToNetwork(pendingSSID, pendingPassword);

      if (result) {
        Serial.println("\t👍 Connection successful!");
      } else {
        Serial.println("\t❌ Connection failed!");
      }

      isConnecting = false;
      connectionInitiated = false;
    } else if (millis() - connectionStartTime > 15000) {
      // Timeout after 15 seconds
      isConnecting = false;
      connectionInitiated = false;
      Serial.println("\t ... Connection attempt timed out");
    }
  }

  // Process DNS in AP mode - needs to run every loop iteration, not just during connection
  if (wifiManager.isAPMode()) {
    wifiManager.processDNS();
  }

  delay(50);
}


// RTOS Task Callback Event: Here used to monitor signal for for ESP resetting
void serialMonitorTask(void *parameter) {
  for (;;) {
    if (Serial.available() > 0) {
      char incomingByte = Serial.read();
      if (incomingByte == 'r') {
        Serial.println("\nRestarting ESP32...\n");
        delay(1000);
        ESP.restart();
      }
    }
#ifdef CAMERA_MODEL_AI_THINKER
    vTaskDelay(30 / portTICK_PERIOD_MS);  // Longer delay for AI-Thinker
#else
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Original delay for XIAO
#endif
  }
}