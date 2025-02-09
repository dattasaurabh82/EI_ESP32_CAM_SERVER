#include "filesystem_handler.h"

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