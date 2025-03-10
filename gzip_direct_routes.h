void setupGzipRoutes(AsyncWebServer &server) {
  // A global counter for cache hits across all assets
  static int totalCacheHits = 0;

  // Root path (/) - with cache tracking
  server.on("/", HTTP_GET, [&totalCacheHits](AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    Serial.println("📄 Serving root path (/)");

    // Check for cache headers
    if (request->hasHeader("If-Modified-Since")) {
      totalCacheHits++;
      AsyncWebServerResponse *response = request->beginResponse(304);
      request->send(response);
      Serial.printf("💾 Cache hit for / (total hits: %d)\n", totalCacheHits);
      return;
    }

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
      response->addHeader("Cache-Control", "max-age=86400");

      // Add Last-Modified header for caching
      response->addHeader("Last-Modified", "Wed, 21 Oct 2024 07:28:00 GMT");

      request->send(response);
      Serial.printf("✅ Served gzipped root index.html (%u bytes)\n", rootAsset->length);
    } else {
      request->send(404, "text/plain", "Root asset not found");
      Serial.println("❌ Root asset not found in GZIP_ASSETS");
    }
    Serial.printf("\nServed root in %lu ms\n", millis() - startTime);
  });

  // Setup routes for each asset - with cache tracking
  for (int i = 0; i < GZIP_ASSETS_COUNT; i++) {
    const String path = GZIP_ASSETS[i].path;
    // Skip root as we've already defined it
    if (path == "/") continue;

    server.on(path.c_str(), HTTP_GET, [i, &totalCacheHits](AsyncWebServerRequest *request) {
      // Check for cache headers
      if (request->hasHeader("If-Modified-Since")) {
        totalCacheHits++;
        AsyncWebServerResponse *response = request->beginResponse(304);
        request->send(response);
        Serial.printf("💾 Cache hit for %s (total hits: %d)\n",
                      GZIP_ASSETS[i].path, totalCacheHits);
        return;
      }

      // Not cached, serve normally
      Serial.printf("📄 Serving asset: %s\n", GZIP_ASSETS[i].path);
      AsyncWebServerResponse *response = request->beginResponse(
        200, GZIP_ASSETS[i].content_type, GZIP_ASSETS[i].data, GZIP_ASSETS[i].length);
      response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "max-age=86400");

      // Add Last-Modified header for caching
      response->addHeader("Last-Modified", "Wed, 21 Oct 2024 07:28:00 GMT");

      request->send(response);
      Serial.printf("✅ Served gzipped asset: %s (%u bytes)\n",
                    GZIP_ASSETS[i].path, GZIP_ASSETS[i].length);
    });

    Serial.printf("✅ Registered route for: %s\n", GZIP_ASSETS[i].path);
  }

  Serial.println("✅ All GZIP routes registered");
}