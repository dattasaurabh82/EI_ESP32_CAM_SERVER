#include "server_handlers.h"
#include "filesystem_handler.h"

// Stream-related constants
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t webServer = NULL;


// Helper function for serving LittleFS files
esp_err_t static_handler(httpd_req_t *req) {
  String path = req->uri;
  String contentType = "text/plain";

  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";

  if (path == "/") path = "/index.html";

  if (LittleFS.exists(path.c_str())) {
    File file = LittleFS.open(path.c_str(), "r");
    httpd_resp_set_type(req, contentType.c_str());
    size_t bytesRead;
    uint8_t buffer[1024];
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
      httpd_resp_send_chunk(req, (const char *)buffer, bytesRead);
    }
    file.close();
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
  }
  return ESP_FAIL;
}

esp_err_t stream_handler(httpd_req_t *req) {
  Serial.println("Stream handler called");
  esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  char *part_buf[64];
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      }
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
      }
      esp_camera_fb_return(fb);
    }
    if (res != ESP_OK) {
      Serial.println("Stream interrupted");
      break;
    }
  }
  return res;
}

esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);

  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 8;

  if (httpd_start(&webServer, &config) == ESP_OK) {
    // URI handler for root/static files
    httpd_uri_t index_uri = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = static_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(webServer, &index_uri);

    // URI handler for stream
    httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(webServer, &stream_uri);

    // URI handler for capture
    httpd_uri_t capture_uri = {
      .uri = "/capture",
      .method = HTTP_GET,
      .handler = capture_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(webServer, &capture_uri);

    Serial.println("HTTP server started on port 80");
  }
}