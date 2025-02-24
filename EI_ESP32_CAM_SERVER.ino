#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define CONFIG_LITTLEFS_CACHE_SIZE 512

#include "wifi_manager.h"
#include "camera_init.h"

#ifdef CAMERA_MODEL_XIAO_ESP32S3
// No brownout includes needed
#elif defined(CAMERA_MODEL_AI_THINKER)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif


AsyncWebServer server(80);  // Single server instance
WifiManager wifiManager;    // Create WiFi manager instance

bool isStreamActive = true;  // Flag to track Streaming State status
bool isConnecting = false;   // Flag to track WiFi connection status

String lastConnectionSSID = "";
String lastConnectionPassword = "";

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
  Serial.println("  Received Save frame request ...");
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
  Serial.println("\n3. WiFi Manager Initialization:");

  // Initialize the WiFi manager
  wifiManager.begin();

  // Try to connect to saved networks first
  if (!wifiManager.connectToSavedNetworks()) {
    // If no saved networks or connection fails, start AP mode
    wifiManager.startAPMode();
  }
}


void setup() {
#ifdef CAMERA_MODEL_XIAO_ESP32S3
  // Skip brownout and pin settings
#elif defined(CAMERA_MODEL_AI_THINKER)
  // Brownout prevention
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Camera power pin stabilization (AI Thinker specific)
  pinMode(12, OUTPUT);  // ESP32-CAM Flash LED pin
  digitalWrite(12, LOW);
  pinMode(10, OUTPUT);  // ESP32-CAM Flash LED pin
  digitalWrite(10, LOW);
#endif

  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  delay(3000);
  Serial.println();
  Serial.println("___ ESP32-CAM-WEB-SERVER - (edgeImpulse tool)___");

  // 1. Cam init
  initCamera();
  // 2. LittleFS init
  initLittleFS();
  // 3. Connect to Wi Fi
  setupWIFIstn();

  // 4. Configure AsyncWebServer Routes
  // Static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
    Serial.println("Client has tried to access ...");
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "application/javascript");
  });
  // server.on("/ei-labeling-guide.png", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/ei-labeling-guide.png", "image/png");
  // });

  // WiFi Portal files
  server.on("/wifi_portal.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/wifi_portal.html", "text/html");
  });

  server.on("/wifi_portal.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/wifi_portal.css", "text/css");
  });

  server.on("/wifi_portal.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/wifi_portal.js", "application/javascript");
  });

  // Stream endpoints
  server.on("/toggleStream", HTTP_POST, [](AsyncWebServerRequest *request) {
    isStreamActive = !isStreamActive;
    Serial.printf("Stream state: %s\n", isStreamActive ? "Active" : "Paused");
    request->send(200, "text/plain", isStreamActive ? "streaming" : "paused");
  });

  server.on("/stream", HTTP_GET, handleMjpeg);

  // Capture endpoint
  server.on("/capture", HTTP_GET, handleCapture);

  // Clear endpoint: Both GET & POST
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("  Received Clear all GET request ...");
    request->send(200, "text/plain", "Images cleared (GET)");
  });
  server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("  Received Clear all POST request ...");
    request->send(200, "text/plain", "Images cleared (POST)");
  });

  server.on("/saveConfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("config", true)) {
      String config = request->getParam("config", true)->value();
      File file = LittleFS.open("/ei_config.json", "w");
      if (file) {
        file.print(config);
        file.close();
        Serial.println("  Configuration saved to LittleFS ...");
        request->send(200, "text/plain", "Configuration saved");
      } else {
        Serial.println("  Failed to save configuration to LittleFS ...");
        request->send(500, "text/plain", "Failed to save configuration");
      }
    } else {
      request->send(400, "text/plain", "No configuration data");
    }
  });

  server.on("/loadConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/ei_config.json")) {
      request->send(LittleFS, "/ei_config.json", "application/json");
      Serial.println("  Configuration loaded from LittleFS ...");
    } else {
      Serial.println("  No configuration found in LittleFS ...");
      request->send(404, "text/plain", "No configuration found");
    }
  });

  // WiFi Manager API endpoints
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

  // server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
  //   String ssid, password;
  //   if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
  //     ssid = request->getParam("ssid", true)->value();
  //     password = request->getParam("password", true)->value();

  //     // Start connection process (non-blocking)
  //     isConnecting = true;

  //     // We need to respond to the client before attempting connection
  //     request->send(200, "text/plain", "Connection attempt started");

  //     // Connection attempt will be handled in the loop() function
  //   } else {
  //     request->send(400, "text/plain", "Missing parameters");
  //   }
  // });
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
  });

  server.begin();
  Serial.println("Async HTTP server started on port 80");
}


void loop() {
  // Monitor HEAP and PSRAM USAGE and apply a more aggressive restart control
  // Note: More easy would be if ESP.getFreeHeap() < 60000
  if (ESP.getFreeHeap() < 20000 || ESP.getFreePsram() < 10000) {
    Serial.printf("Free PSRAM: %lu bytes\n", ESP.getFreePsram());
    Serial.printf("Free Heap: %lu bytes\n\n", ESP.getFreeHeap());
    Serial.println("Low memory: Restarting\n");
    ESP.restart();
  }

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
        Serial.println("Connection successful!");
      } else {
        Serial.println("Connection failed!");
      }

      isConnecting = false;
      connectionInitiated = false;
    } else if (millis() - connectionStartTime > 15000) {
      // Timeout after 15 seconds
      isConnecting = false;
      connectionInitiated = false;
      Serial.println("Connection attempt timed out");
    }
  }

  // Process DNS in AP mode - needs to run every loop iteration, not just during connection
  if (wifiManager.isAPMode()) {
    wifiManager.processDNS();
  }

  delay(100);
}