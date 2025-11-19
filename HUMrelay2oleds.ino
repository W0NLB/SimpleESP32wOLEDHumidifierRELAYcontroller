// ESP32 Relay Controller — Dual OLED (SSD1306 @0x3C, SH1106 @0x3D)
// Relay on GPIO15 (D15). OLED SDA=21, SCL=22
// Accepts GET /relay?state=on or /relay?state=off (also accepts /on and /off)

#include <WiFi.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <SH1106Wire.h>

#define RELAY_PIN 15

// I2C pins for ESP32
#define OLED_SDA 21
#define OLED_SCL 22

// Displays
SSD1306Wire displaySSD(0x3C, OLED_SDA, OLED_SCL); // 0.96" SSD1306
SH1106Wire  displaySH (0x3D, OLED_SDA, OLED_SCL); // 1.3" SH1106

// WiFi credentials
const char* ssid = "SIGINT_KeyHole7";
const char* password = "M781166s";

WiFiServer server(80);
bool relayState = false;
String wifiStatus = "WiFi: ?";

void drawBoth(String a, String b, String c, String d) {
  // SSD1306
  displaySSD.clear();
  displaySSD.setFont(ArialMT_Plain_10);
  displaySSD.drawString(0, 0, a);
  displaySSD.drawString(0, 12, b);
  displaySSD.drawString(0, 24, c);
  displaySSD.drawString(0, 36, d);
  displaySSD.display();

  // SH1106
  displaySH.clear();
  displaySH.setFont(ArialMT_Plain_10);
  displaySH.drawString(0, 0, a);
  displaySH.drawString(0, 12, b);
  displaySH.drawString(0, 24, c);
  displaySH.drawString(0, 36, d);
  displaySH.display();
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = "WiFi: Connected";
    return;
  }
  wifiStatus = "WiFi: Reconnecting...";
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
  }
  wifiStatus = (WiFi.status() == WL_CONNECTED) ? "WiFi: Connected" : "WiFi: Failed";
}

void handleClient(WiFiClient client) {
  // Read request line
  String req = client.readStringUntil('\r');
  client.flush();
  Serial.println("Req: " + req);

  // Simple parsing
  if (req.indexOf("/relay?state=on") >= 0 || req.indexOf("/on") >= 0) {
    digitalWrite(RELAY_PIN, HIGH);
    relayState = true;
  } else if (req.indexOf("/relay?state=off") >= 0 || req.indexOf("/off") >= 0) {
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
  }

  // Send minimal HTTP response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println(relayState ? "Humidifier ON" : "Humidifier OFF");
  delay(10);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Init displays
  displaySSD.init();
  displaySH.init();
  displaySSD.flipScreenVertically();
  displaySH.flipScreenVertically();

  // WiFi
  WiFi.begin(ssid, password);
  wifiStatus = "WiFi: Connecting";
  while (WiFi.status() != WL_CONNECTED) {
    drawBoth("Relay Controller", "Connecting WiFi...", "", "");
    delay(400);
  }
  wifiStatus = "WiFi: Connected";

  server.begin();

  drawBoth("Relay Controller",
           "State: Humidifier OFF",
           "IP: " + WiFi.localIP().toString(),
           wifiStatus);
}

void loop() {
  ensureWiFi();

  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
    // update display after handling request
    drawBoth("Relay Controller",
             relayState ? "State: ON" : "State: OFF",
             "IP: " + WiFi.localIP().toString(),
             wifiStatus);
  } else {
    // keep display current
    drawBoth("Relay Controller",
             relayState ? "State: ON" : "State: OFF",
             "IP: " + WiFi.localIP().toString(),
             wifiStatus);
    delay(200);
  }
}
