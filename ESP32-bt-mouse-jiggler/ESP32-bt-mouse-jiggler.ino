/**
 * This program turns the ESP32 into a Bluetooth LE mouse that moves up and down every 60 seconds
 * and that way prevents the computer going into idle/sleep mode.
 */
#include <BleMouse.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define LED_BUILTIN 2
#define WIFI_SSID "Your Wi-Fi SSID"
#define WIFI_PASSWORD "Your Wi-Fi password"

BleMouse bleMouse;
WebServer server(80);
bool enabled = true;
bool reboot = false;
unsigned int cycleCounter = 0;
unsigned int rebootCounter = 0;

/**
 * Blink builtin LED
 */
void blink(int ms) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ms / 2);
  digitalWrite(LED_BUILTIN, LOW);
  delay(ms / 2);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    blink(500);
    Serial.print(".");
  }

  // Enable Bonjour device discovery
  if (MDNS.begin("esp32")) {
    for (int i = 0; i < 10; i++) {
      blink(100);
    }
    Serial.println("MDNS responder started");
  }

  // Simple web server listening on port 80, allows remote enabling or disabling the jiggler
  // and also rebooting of ESP32 board in case it gets stuck (sometimes it does not connect
  // as bluetooth device after powering up the computer)
  server.on("/", []() {
    char message[256] = "Bluetooth Jiggler ";
    if (enabled) {
      strcat(message, "enabled <button onclick=\"location.href='/disable'\" type=\"button\">Disable Jiggler</button>");
    } else {
      strcat(message, "disabled <button onclick=\"location.href='/enable'\" type=\"button\">Enable Jiggler</button>");
    }
    if (bleMouse.isConnected()) {
      strcat(message, "<br/>Connected to PC");
    } else {
      strcat(message, "<br/>Disonnected from PC");
    }
    strcat(message, "<hr/><button onclick=\"location.href='/reboot'\" type=\"button\">Reboot ESP32</button>");
    server.sendHeader("Date", "Thu, 1 Jan 2099 12:00:00 GMT", true);
    server.sendHeader("Expires", "0", true);
    server.send(200, "text/html", message);
  });

  server.on("/enable", []() {
    Serial.println("Enabling Bluetooth Jiggler");
    enabled = true;
    server.sendHeader("Location", "/", true);
    server.sendHeader("Date", "Thu, 1 Jan 2099 12:00:00 GMT", true);
    server.sendHeader("Expires", "0", true);
    server.send(301, "text/html", "");
  });

  server.on("/disable", []() {
    Serial.println("Disabling Bluetooth Jiggler");
    enabled = false;
    cycleCounter = 0;
    server.sendHeader("Location", "/", true);
    server.sendHeader("Date", "Thu, 1 Jan 2099 12:00:00 GMT", true);
    server.sendHeader("Expires", "0", true);
    server.send(301, "text/html", "");
  });

  server.on("/reboot", []() {
    Serial.println("Pending Reboot...");
    server.sendHeader("Location", "/", true);
    server.sendHeader("Date", "Thu, 1 Jan 2099 12:00:00 GMT", true);
    server.sendHeader("Expires", "0", true);
    server.send(301, "text/html", "");
    reboot = true;
  });

  server.begin();
  bleMouse.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(2);

  if (enabled && bleMouse.isConnected()) {
    digitalWrite(LED_BUILTIN, LOW);
    if (++cycleCounter >= 10000) {
      Serial.println("Mouse move");
      bleMouse.move(0, -1);  // up
      bleMouse.move(0, +1);  // down
      blink(100);
      cycleCounter = 0;
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (reboot) {
    if (++rebootCounter >= 1000) {
      Serial.println("Rebooting");
      ESP.restart();
    }
  }
}