#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Switch to LittleFS if needed
#define USE_LittleFS

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
  //#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#else
  #include <SPIFFS.h>
#endif

#ifndef webServer_h
#define webServer_h

// Init webserver
AsyncWebServer server(80);

// Replace placeholders in HTML files
String webServerVarHandler(const String& var) {

  // Unfortunately, var can only be handled with if conditions in this scenario.
  // A switch..case statement cannot be used with datatype "string".
  if (var == "TIME") {
    // System time
    String dummy = "TIME_VAR";
    return dummy;
  } else if (var == "RTC_TIME") {
    // Current RTC time
    String dummy = "TIME_VAR";
    return dummy;

  } else if (var == "NTP_SOURCE") {
    // Current NTP server
    String dummy = "TIME_VAR";
    return dummy;

  } else if (var == "HUE_BRIDGE") {
    // Hue bridge IP
    String dummy = "TIME_VAR";
    return dummy;

  } else if (var == "HUE_TOGGLEON_TIME") {
    // Hue toggle on time
    String dummy = "TIME_VAR";
    return dummy;

  } else if (var == "HUE_TOGGLEOFF_TIME") {
    // Hue toggle off time
    String dummy = "TIME_VAR";
    return dummy;

  } else if (var == "TUBES_DISPLAY") {
    // Hue toggle off time
    String dummy = "TIME_VAR";
    return dummy;

  }

  return String();
}

void webServerRequestHandler() {
  // Root / Index
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /."));

      if(SPIFFS.open("/htmlRoot.html")) {
        request->send(SPIFFS, "/htmlRoot.html", "text/html", false, webServerVarHandler);
      } else {
        Serial.println("[X] webServer: GET / - No local ressource.");
      }
  });
  
  // Tube control
  server.on("/tube", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /tube."));

      if(SPIFFS.open("/htmlTubes.html")) {
        request->send(SPIFFS, "/htmlTubes.html", "text/html", false, webServerVarHandler);
      } else {
        Serial.println("[X] webServer: GET /tube - No local ressource.");
      }
  });

  // RTC control
  server.on("/rtc", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /rtc."));

      if(SPIFFS.open("/htmlRTC.html")) {
        request->send(SPIFFS, "/htmlRTC.html", "text/html", false, webServerVarHandler);
      } else {
        Serial.println("[X] webServer: GET /rtc - No local ressource.");
      }      
  });

  // Philips HUE control
  server.on("/hue", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /hue."));

      if(SPIFFS.open("/htmlHUE.html")) {
        request->send(SPIFFS, "/htmlHUE.html", "text/html", false, webServerVarHandler);
      } else {
        Serial.println("[X] webServer: GET /hue - No local ressource.");
      }
  });

  // Error pages
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println(F("[T] WebServer: GET - 404."));
    request->send(404, "text/plain", "Content not found.");
  });

  // Start the webserver
  server.begin();
  Serial.println(F("[T] WebServer: Start."));
}

#endif