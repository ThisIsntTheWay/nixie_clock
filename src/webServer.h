#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include <rtc.h>
#include <philipsHue.h>

// Switch to LittleFS if needed
#define USE_LittleFS

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 

#ifndef webServer_h
#define webServer_h

// Init webserver
AsyncWebServer server(80);

//  ---------------------
//  FUNCTIONS
//  ---------------------

// Replace placeholders in HTML files
String processor(const String& var) {
  Serial.print("PROCESSOR: ");
    Serial.println(var);

  // Unfortunately, var can only be handled with if conditions in this scenario.
  // A switch..case statement cannot be used with datatype "string".
  if (var == "TIME" || var == "RTC_TIME") {
    return getTime();

  } else if (var == "NTP_SOURCE") {
    // Current NTP server
    return parseRTCconfig(1);

  } else if (var == "TIME_MODE") {
    // Current NTP server
    return parseRTCconfig(2);

  } else if (var == "HUE_BRIDGE") {
    // Hue bridge IP
    return parseHUEconfig(1);

  } else if (var == "HUE_TOGGLEON_TIME") {
    // Hue toggle on time
    return parseHUEconfig(2);

  } else if (var == "HUE_TOGGLEOFF_TIME") {
    // Hue toggle off time
    return parseHUEconfig(3);

  } else if (var == "TUBES_DISPLAY") {
    // Hue toggle off time
    String dummy = "1 2 3 4";
    return dummy;

  }

  return String();
}

/*
DOES NOT WORK YET :(

void serveWebRequest(char &fs, String& path, AsyncWebServerRequest *request) {
      Serial.print(F("[T] WebServer: GET "));
        Serial.println(path);

      if(LITTLEFS.open(fs)) {
        request->send(LITTLEFS, fs, "text/html", false, webServerVarHandler);
      } else {
        Serial.print("[X] WebServer: GET ");
          Serial.print(path);
          Serial.println(" - No local ressource.");
      }
}
*/

//  ---------------------
//  TASKS
//  ---------------------

void webServerStartup() {

  // Wait for FlashFS
  while (!FlashFSready) { vTaskDelay(500); }
  LITTLEFS.begin();

  // ----------------------------
  // static content

  // Root / Index
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /."));

      String f = "/html/index.html";

      if(LITTLEFS.exists(f)) {
        request->send(LITTLEFS, "/html/index.html", String(), false, processor);
        //request->send(LITTLEFS, f);
      } else {
        Serial.println("[X] WebServer: GET / - No local ressource.");
      }
  });
  
  // Tube control
  server.on("/tube", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /tube."));

      String f = "/html/cfgTUBE.html";

      if(LITTLEFS.exists(f)) {
        request->send(LITTLEFS, f, String(), false, processor);
      } else {
        Serial.println("[X] WebServer: GET /tube - No local ressource.");
      }
  });

  // RTC control
  server.on("/rtc", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /rtc."));

      String f = "/html/cfgRTC.html";

      if(LITTLEFS.exists(f)) {
        request->send(LITTLEFS, f, String(), false, processor);
      } else {
        Serial.println("[X] WebServer: GET /rtc - No local ressource.");
      }      
  });

  // Philips HUE control
  server.on("/hue", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /hue."));

      String f = "/html/cfgHUE.html";

      if(LITTLEFS.exists(f)) {
        request->send(LITTLEFS, f, String(), false, processor);
      } else {
        Serial.println("[X] WebServer: GET /hue - No local ressource.");
      }
  });

  // Serve favicon
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LITTLEFS, "/html/favicon.png", "image/png");
  });

  // ----------------------------
  // Endpoints
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/RTCendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) {
      data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
      data = json.as<JsonObject>();
    }
    
    // Save JSON response as variables
    const char* responseM = data["mode"];
    const char* responseV = data["value"];

    // Serialize JSON
    String response;
    serializeJson(data, response);

    // Read file
    StaticJsonDocument<200> tmpJSON;
    File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

    DeserializationError error = deserializeJson(tmpJSON, rtcConfig);
    if (error) {
        Serial.println(F("[X] WebServer: Could not deserialize JSON."));
        request->send(400, "text/html", "<p style='color: red;'>Cannot deserialize JSON.</p>");
    } else {
      rtcConfig.close();

      // Write to file based on request body
      tmpJSON["Mode"] = responseM;

      if (data["mode"] == "ntp") {
        tmpJSON["NTP"] = responseV;
      } 
      else if (data["mode"] == "manual") {
        tmpJSON["manualTime"] = responseV;
        rtc.adjust(responseV);
      }

      // Write rtcConfig.cfg
      File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");
      if (!(serializeJson(tmpJSON, rtcConfig))) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "text/html", "<p style='color: red;'>Cannot write to config.</p>");
      } else {
        request->send(200, "text/html", "<p style='color: green;'>OK</p>");
      }
    }

    rtcConfig.close();

    Serial.println(response);
  });
  server.addHandler(handler);

  // ----------------------------
  // Plaintext content

  // Error pages
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println(F("[T] WebServer: GET - 404."));
    request->send(404, "text/html", "<center><h1>Content not found.");
  });
  
  // Test
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "server OK.");
  });

  // Start the webserver
  server.begin();
  Serial.println(F("[T] WebServer: Start."));
}

#endif