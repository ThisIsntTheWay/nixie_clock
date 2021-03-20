#include <FS.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#ifndef webServer
#define webServer

// Init webserver
AsyncWebServer server(80);

// Replace placeholders in HTML files
String webServerVarHandler(const String& var) {
  Serial.println(var);
  if(var == "STATE") {
    String dummy = "d";
    return dummy;
  }

  return String();
}

void webServerRequestHandler() {
  // Root / Index
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /."));

      if(SPIFFS.open("/htmlRoot.html")) {
        request->send(SPIFFS, "/htmlRoot.html", "text/html");
      } else {
        Serial.println("[X] webServer: GET / - No local ressource.");
      }
  });
  
  // Tube control
  server.on("/tube", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /tube."));

      if(SPIFFS.open("/htmlTubes.html")) {
        request->send(SPIFFS, "/htmlTubes.html", "text/html");
      } else {
        Serial.println("[X] webServer: GET /tube - No local ressource.");
      }
  });

  // RTC control
  server.on("/rtc", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /rtc."));

      if(SPIFFS.open("/htmlRTC.html")) {
        request->send(SPIFFS, "/htmlRTC.html", "text/html");
      } else {
        Serial.println("[X] webServer: GET /rtc - No local ressource.");
      }      
  });

  // Philips HUE control
  server.on("/hue", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /hue."));

      if(SPIFFS.open("/htmlHUE.html")) {
        request->send(SPIFFS, "/htmlHUE.html", "text/html");
      } else {
        Serial.println("[X] webServer: GET /hue - No local ressource.");
      }
  });

  // Error pages
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.print(F("[T] WebServer: GET - 404."));
      Serial.println();
    request->send(404, "text/plain", "Content not found.");
  });

  // Start the webserver
  server.begin();
  Serial.println(F("[T] WebServer: Start."));
}

#endif