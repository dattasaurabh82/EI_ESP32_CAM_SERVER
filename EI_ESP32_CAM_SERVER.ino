#include <WiFi.h>
#include "credentials.h"
#include "filesystem_handler.h"
#include "server_handlers.h"
#include "camera_init.h"
#include "soc/soc.h"           //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

// WebServer server(80);        // Main web interface
// WebServer streamServer(81);  // Just for MJPEG stream
// httpd_handle_t webServer = NULL;

// LittleFS file server
// void serveFile(const char* path, const char* contentType) {
//   if (LittleFS.exists(path)) {
//     File file = LittleFS.open(path, "r");
//     server.streamFile(file, contentType);
//     file.close();
//   } else {
//     server.send(404, "text/plain", "File not found");
//   }
// }


// // MJPEG streaming
// void handleMjpeg() {
//   Serial.println("Stream handler called on port 81");
//   WiFiClient client = streamServer.client();  // Changed from server to streamServer
//   if (!client.connected()) {
//     Serial.println("No client connected to stream");
//     return;
//   }
//   // Send HTTP headers
//   client.println("HTTP/1.1 200 OK");
//   client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
//   client.println("Connection: keep-alive");
//   client.println("Access-Control-Allow-Origin: *");
//   client.println();
//   // Continue streaming as long as client is connected
//   while (true) {
//     camera_fb_t* fb = esp_camera_fb_get();
//     if (!fb) {
//       Serial.println("Camera capture failed");
//       delay(500);
//       continue;
//     }
//     // Send frame boundary and headers
//     client.println("--frame");
//     client.println("Content-Type: image/jpeg");
//     client.printf("Content-Length: %d\r\n", fb->len);
//     client.println();
//     // Send frame data
//     client.write(fb->buf, fb->len);
//     client.println();
//     // Return frame buffer
//     esp_camera_fb_return(fb);

//     // Check if client is still connected
//     if (!client.connected()) {
//       Serial.println("Client disconnected");
//       break;
//     }
//     // Small delay to control frame rate
//     delay(50);
//   }
// }


// // Image saving
// void handleCapture() {
//   camera_fb_t* fb = esp_camera_fb_get();
//   if (!fb) {
//     server.send(500, "text/plain", "Camera capture failed");
//     return;
//   }

//   server.sendHeader("Content-Type", "image/jpeg");
//   server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
//   server.sendHeader("Access-Control-Allow-Origin", "*");

//   // Stream directly from buffer
//   WiFiClient client = server.client();
//   client.write(fb->buf, fb->len);

//   esp_camera_fb_return(fb);
// }

// Camera init with verbose output
void initCamera() {
  Serial.println("\n1. Checking Camera Status:");
  Serial.print("   Initializing camera... ");

  if (setupCamera()) {
    Serial.println("✓ Success");

    sensor_t* sensor = esp_camera_sensor_get();
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
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(3000);
  Serial.println();
  Serial.println("___ ESP32-CAM-WEB-SERVER - (edgeImpulse tool)___");

  // 1. Cam init
  initCamera();

  // 2. LittleFS init
  initLittleFS();

  // 3. Connect to WiFi
  setupWIFIstn();

  // 4. Start server
  startCameraServer();

  // // 3. Define web routes for main server (port 80)
  // server.on("/", HTTP_GET, []() {
  //   serveFile("/index.html", "text/html");
  // });
  // server.on("/styles.css", HTTP_GET, []() {
  //   serveFile("/styles.css", "text/css");
  // });
  // server.on("/script.js", HTTP_GET, []() {
  //   serveFile("/script.js", "application/javascript");
  // });

  // // capture and clear on main server
  // server.on("/capture", HTTP_GET, handleCapture);
  // server.on("/clear", HTTP_POST, []() {
  //   server.send(200, "text/plain", "Images cleared");
  // });

  // // 404 handler for main server
  // server.onNotFound([]() {
  //   server.send(404, "text/plain", "404: Not found");
  // });

  // // Setup stream server (port 81)
  // streamServer.on("/stream", HTTP_GET, handleMjpeg);
  // // 404 handler for stream server
  // streamServer.onNotFound([]() {
  //   streamServer.send(404, "text/plain", "404: Not found");
  // });

  // // Start both servers
  // // Start both servers
  // server.begin();
  // Serial.println("   ⤷ Main HTTP server started on port 80");

  // streamServer.begin();
  // Serial.println("   ⤷ MJPEG server started on port 81");
}

void loop() {
  // server.handleClient();
  // streamServer.handleClient();
  delay(2);
}