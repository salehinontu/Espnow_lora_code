// ============================================================
// 🔍 MAC ADDRESS FINDER
// ============================================================
// এই sketch দুইটা ESP32 (Air এবং Ground) এ আলাদা আলাদা upload করো।
// Serial Monitor (baud 115200) খোলো এবং দেখো MAC address।
// দুইটা MAC কাগজে লিখে রাখো — Air এর code এ Ground MAC লাগবে,
// Ground এর code এ Air MAC লাগবে।
// ============================================================

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("  ESP32 MAC ADDRESS FINDER");
  Serial.println("================================");
  Serial.print("MY MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("================================");
  Serial.println();
  Serial.println("📋 Copy this MAC and paste in the");
  Serial.println("   OTHER unit's code.");
  Serial.println();
  Serial.println("Format for code:");

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("uint8_t mac[] = {");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(", ");
  }
  Serial.println("};");
  Serial.println();
}

void loop() {
  // Every 5 sec, repeat
  delay(5000);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}
