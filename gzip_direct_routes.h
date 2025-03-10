#ifndef GZIP_DIRECT_ROUTES_H
#define GZIP_DIRECT_ROUTES_H

#include <ESPAsyncWebServer.h>
#include "gzipped_assets.h"

void setupGzipRoutes(AsyncWebServer &server) {
  // Root path (/) handler
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    Serial.println("📄 Serving root path (/)");

    // Find the root asset
    const GzipAsset *rootAsset = nullptr;
    for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
      if (String(GZIP_ASSETS[i].path) == "/") {
        rootAsset = &GZIP_ASSETS[i];
        break;
      }
    }

    if (rootAsset) {
      AsyncWebServerResponse *response = request->beginResponse(
        200, rootAsset->content_type, rootAsset->data, rootAsset->length);

      response->addHeader("Content-Encoding", "gzip");
      // Simple cache control - either use this for production:
      // response->addHeader("Cache-Control", "max-age=3600");
      // Or this for development:
      response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");

      request->send(response);
      Serial.printf("✅ Served gzipped root index.html (%u bytes)\n", rootAsset->length);
    } else {
      request->send(404, "text/plain", "Root asset not found");
      Serial.println("❌ Root asset not found in GZIP_ASSETS");
    }

    Serial.printf("Served root in %lu ms\n", millis() - startTime);
  });

  // Setup routes for each asset
  for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
    const String path = GZIP_ASSETS[i].path;
    if (path == "/") continue;  // Skip root as we've already defined it

    server.on(path.c_str(), HTTP_GET, [i](AsyncWebServerRequest *request) {
      Serial.printf("📄 Serving asset: %s\n", GZIP_ASSETS[i].path);

      AsyncWebServerResponse *response = request->beginResponse(
        200, GZIP_ASSETS[i].content_type, GZIP_ASSETS[i].data, GZIP_ASSETS[i].length);

      response->addHeader("Content-Encoding", "gzip");
      // Simple cache control - either use this for production:
      // response->addHeader("Cache-Control", "max-age=3600");
      // Or this for development:
      response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");

      request->send(response);
      Serial.printf("✅ Served gzipped asset: %s (%u bytes)\n",
                    GZIP_ASSETS[i].path, GZIP_ASSETS[i].length);
    });

    Serial.printf("✅ Registered route for: %s\n", GZIP_ASSETS[i].path);
  }

  Serial.println("✅ All GZIP routes registered");
}

#endif  // GZIP_DIRECT_ROUTES_H