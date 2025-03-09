// gzip_handler.h
#ifndef GZIP_HANDLER_H
#define GZIP_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "gzipped_assets.h"

class GzipWebHandler : public AsyncWebHandler {
public:
  GzipWebHandler() {}

  virtual ~GzipWebHandler() {}

  bool canHandle(AsyncWebServerRequest *request) override {
    // First check if this is a special dynamic file request
    if (request->method() == HTTP_GET) {
      if (request->url() == "/ei_config.json") {
        return true;  // Handle dynamically from LittleFS
      }

      // Then check for gzipped static assets
      for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
        if (request->url() == GZIP_ASSETS[i].path || (request->url() == "/" && String(GZIP_ASSETS[i].path) == "/")) {
          return true;
        }
      }
    }
    return false;
  }

  void handleRequest(AsyncWebServerRequest *request) override {
    // Special handling for dynamic config file
    if (request->url() == "/ei_config.json") {
      handleConfigFile(request);
      return;
    }

    // Find the requested asset in the GZIP table
    const GzipAsset *asset = nullptr;

    for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
      if (request->url() == GZIP_ASSETS[i].path || (request->url() == "/" && String(GZIP_ASSETS[i].path) == "/")) {
        asset = &GZIP_ASSETS[i];
        break;
      }
    }

    if (!asset) {
      return request->send(404);
    }

    // Send the gzipped asset with appropriate headers
    AsyncWebServerResponse *response = request->beginResponse_P(
      200, asset->content_type, asset->data, asset->length);

    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);

    Serial.printf("\tServed gzipped asset: %s (%u bytes)\n",
                  asset->path, asset->length);
  }

private:
  void handleConfigFile(AsyncWebServerRequest *request) {
    // Check if file exists in LittleFS
    if (LittleFS.exists("/ei_config.json")) {
      request->send(LittleFS, "/ei_config.json", "application/json");
      Serial.println("\tConfiguration loaded from LittleFS ...");
    } else {
      // Fall back to template if actual config doesn't exist
      for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
        if (String(GZIP_ASSETS[i].path) == "/ei_config.template.json") {
          AsyncWebServerResponse *response = request->beginResponse_P(
            200, "application/json", GZIP_ASSETS[i].data, GZIP_ASSETS[i].length);

          response->addHeader("Content-Encoding", "gzip");
          request->send(response);

          Serial.println("\tServed default config template (no config found) ...");
          return;
        }
      }

      // If we get here, we couldn't find the template either
      request->send(404, "text/plain", "No configuration found");
      Serial.println("\tNo configuration found in LittleFS ...");
    }
  }
};

#endif  // GZIP_HANDLER_H