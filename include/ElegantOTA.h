#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "credentials.h"

//const char* mySSID = "........";
//const char* myPASSWORD = ".............";

// Try and get a static IP address (so we can find the device for web pages)
//IPAddress local_IP(192, 168, 1, 222); // change 103 to be unique on your system
//IPAddress gateway(192, 168, 1, 254);
//IPAddress subnet(255, 255, 255, 0);

// To get the local time we must include DNS servers. Google's used here.
//IPAddress primaryDNS(8, 8, 8, 8);
//IPAddress secondaryDNS(8, 8, 4, 4);

// Forward declaration: convert Wi-Fi connection response to meaningful message
//const char *wl_status_to_string(wl_status_t status);

// Standard web server, on port 80. Must be global. Obvs.
AsyncWebServer server(80);

// All Wi-Fi and server setup is done here
void ElOTA()
{
  Serial.begin(115200);
  Serial.println("server up");

  // Assuming you want your ESP32 to act like any other client (STAtion) on your Wi-Fi
  WiFi.mode(WIFI_STA);

  // Store Wi-Fi configuration in EEPROM? Not a good idea as it will never forget these settings.
  WiFi.persistent(false);

  // Reconnect if connection is lost
  WiFi.setAutoReconnect(false);

  // Modem sleep when in WIFI_STA mode not a good idea as someone might want to talk to it
  WiFi.setSleep(false);
  Serial.println("no sleep");

  // Let's do it
  //WiFi.config(gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(mySSID, myPASSWORD);
  
  delay(2000);

  // Wait for connection
  if (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("...No OTA...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(mySSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  // Now register some server pages

  // Just to be sure we have a 404 response on unknown requests
  server.onNotFound(
      [](AsyncWebServerRequest *request) {
        // Send back a plain text message (can be made html if required)
        request->send(404, "text/plain", "404 - Page Not Found, oops!");
      });

  // Send back a web page (landing page)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Tell the system that we will be sending back html (not just plain text)
    AsyncResponseStream *response = request->beginResponseStream("text/html");

    // Format the html as a RAW literal so we don't have to escape all the quotes (and more)
    response->printf(R"(
      <html>
        <head>
          <title>Landing Page</title>
          <style>
            body {
              background-color: yellow;
              font-family: Arial, Sans-Serif;
            }
          </style>
        </head>
        <body>
          <h1>Landing Page</h1>
          <p>--------------------------------------------------- </p>
          <p>MCU:    TTGO T-OI PLUS RISC-V ESP32-C3 </p>
          <p>Model:  EANx O2 Upcycler </p>
          <p>File:   EANx_Upcycler on Platform IO </p>
          <p>---------------------------------------------------  </p>
          <p>
            Looking for update?
          </p>
        </body>
      </html>
    )");

    // Send back all the text/html for a standard web page
    request->send(response);
  });

  // Starting Async OTA web server AFTER all the server.on requests registered
  AsyncElegantOTA.begin(&server);
  Serial.println("OTA begin");
  server.begin();
  Serial.println("server begin");
}

// Translates the Wi-Fi connect response to English
const char *wl_status_to_string(wl_status_t status)
{
  switch (status) {
    case WL_NO_SHIELD:
      return "WL_NO_SHIELD";
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}