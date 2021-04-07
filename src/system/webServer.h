/*
    ESP32 Nixie Clock - Webserver module
    (c) V. Klopfenstein, 2021

    This code block spawns an instance of ESPAsyncWebserver.
    Anything web-related occurs in here.
*/

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"

#include <system/rtc.h>
#include <system/philipsHue.h>
#include <utils/utilities.h>

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

  } else if (var == "NTP_SOURCE") { // Current NTP server
    return parseRTCconfig(1);

  } else if (var == "TIME_MODE") { // Current NTP server
    return parseRTCconfig(2);

  } else if (var == "GMT_VAL") { // Current DMT
    return parseRTCconfig(3);

  } else if (var == "DST_VAL") { // Current DST
    return parseRTCconfig(4);

  } else if (var == "HUE_BRIDGE") { // Hue bridge IP
    return parseHUEconfig(1);

  } else if (var == "HUE_API_USER") { // Hue API User   
    //Serial.println(parseHUEconfig(2));
    return "* * *";

  } else if (var == "HUE_TOGGLEON_TIME") { // Hue toggle on time
    return parseHUEconfig(3);

  } else if (var == "HUE_TOGGLEOFF_TIME") { // Hue toggle off time
    return parseHUEconfig(4);

  } else if (var == "TUBES_DISPLAY") { // Hue toggle off time
    String dummy = "1 2 3 4";
    return dummy;
  }

  return String();
}

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
  
  // Debug interface
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println(F("[T] WebServer: GET /debug."));

      String f = "/html/debug.html";

      if(LITTLEFS.exists(f)) {
        request->send(LITTLEFS, f, String(), false, processor);
      } else {
        Serial.println("[X] WebServer: GET /debug - No local ressource.");
      }
  });

  // Serve favicon
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LITTLEFS, "/html/favicon.png", "image/png");
  });

  // ----------------------------
  // Endpoints
  // ----------------------------

  // ============
  // RTC ENDPOINT
  
  AsyncCallbackJsonWebHandler *rtchandler = new AsyncCallbackJsonWebHandler("/RTCendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }
    
    // Save JSON response as variables
    const char* rM = data["mode"];
    const char* rV = data["value"];
    const char* rGMT = data["gmt"];
    const char* rDST = data["dst"];

    // Serialize JSON
    String response;
    serializeJson(data, response);

    // Read file
    StaticJsonDocument<200> tmpJSON;
    File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

    DeserializationError error = deserializeJson(tmpJSON, rtcConfig);
    if (error) {
      Serial.println(F("[X] WebServer: Could not deserialize JSON."));
      request->send(400, "application/json", "{'status': 'error', 'message': 'Cannot deserialize JSON!'}");
    } else {
      rtcConfig.close();

      String errMsg = String("Cannot write to config!");
      bool InputValid = true;

      // Write to file based on request body
      tmpJSON["Mode"] = rM;

      // NTP mode
      if (data["mode"] == "ntp") {
        if (data.containsKey("value")) {
          if (validateEntry(rV, 1, 4)) { tmpJSON["NTP"] = rV; }
          else { InputValid = false; errMsg = errMsg + String(" Server bad format."); }
        }
        
        if (data.containsKey("gmt")) {
          if (validateEntry(rGMT, 1, 4)) { tmpJSON["GMT"] = rGMT; }
          else { InputValid = false; errMsg = errMsg + String(" GMT bad format."); }
        }

        if (data.containsKey("dst")) {
          if (validateEntry(rDST, 1, 4)) { tmpJSON["DST"] = rDST; }
          else { InputValid = false; errMsg = errMsg + String(" DST bad format."); }
        }   
      }

      // Manual mode
      else if (data["mode"] == "manual") {
        tmpJSON["manualTime"] = rV;
        rtc.adjust(rV);
      } else {
        InputValid = false;
        errMsg = errMsg + String(" No mode specified.");
      }

      // Write rtcConfig.cfg
      File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");
      if (!(serializeJson(tmpJSON, rtcConfig)) || !InputValid) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{'status': 'error', 'message': '" + errMsg + "'}");
      } else {
        request->send(200, "application/json", "{'status': 'success', 'message': 'Config was updated.'}");
      }
    }

    rtcConfig.close();

    Serial.println(response);
  });
  server.addHandler(rtchandler);

  // ============
  // HUE ENDPOINT
  
  AsyncCallbackJsonWebHandler *huehandler = new AsyncCallbackJsonWebHandler("/HUEendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }
    
    // Save JSON response as variables
    const char* rIP = data["bridgeip"];
    const char* rUSR = data["apiuser"];
    const char* rON = data["ontime"];
    const char* rOFF = data["offtime"];

    /*Serial.print(rOFF);
      Serial.print(" ");
      LEN = 0;
      while (rON[LEN] != 0) LEN++;
      Serial.println(LEN);*/

    // Serialize JSON
    String response;
    serializeJson(data, response);

    // Read file
    StaticJsonDocument<200> tmpJSON;
    File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "r");

    DeserializationError error = deserializeJson(tmpJSON, hueConfig);
    if (error) {
      Serial.println(F("[X] WebServer: Could not deserialize JSON."));
        request->send(400, "application/json", "{'status': 'error', 'message': 'Cannot deserialize JSON!'}");
    } else {
      hueConfig.close();

      String errMsg = String("Config write failure.");
      bool InputValid = true;

      // Validate entries and change if needed
      // Skip empty data fields
      int e = 0;
      if (data.containsKey("bridgeip")) {
        if (!(data["bridgeip"] == "NaN")) {
          if (validateEntry(rIP, 1, 7)) { tmpJSON["IP"] = rIP; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rIP bad format. ");
          }
        }
      } else { e++; Serial.println("rIP validation failure.");}
      
      if (data.containsKey("apiuser")) {
        if (!(data["apiuser"] == "NaN")) {
          if (validateEntry(rUSR, 1, 7)) { tmpJSON["user"] = rUSR; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rUSR bad format. ");
          }
        }
      } else { e++; Serial.println("rUSR validation failure.");}

      if (data.containsKey("ontime")) {
        if (!(data["ontime"] == "NaN")) {
          if (validateEntry(rON, 1, 4)) { tmpJSON["toggleOnTime"] = rON; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rOn bad format. ");
          }
        }
      } else { e++; Serial.println("rOn validation failure.");}

      if (data.containsKey("offtime")) {
        if (!(data["offtime"] == "NaN")) {
          if (validateEntry(rOFF, 1, 4)) { tmpJSON["toggleOffTime"] = rOFF; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rOff bad format.");
          }
        }
      } else { e++; Serial.println("rOff validation failure.");}

      // Write to file based on request body
      // Produce error if 'e' is equal to 4, InputValid is false or JSON could not get serialized
      File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "w");
      if ( !(serializeJson(tmpJSON, hueConfig)) || !InputValid || (e == 4)) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{'status': 'error', 'message': '" + errMsg + "'}");
      } else {
        request->send(200, "application/json", "{'status': 'success', 'message': 'Config was updated.'}");
      }
    }

    hueConfig.close();

    Serial.println(response);
  });
  server.addHandler(huehandler);

  server.on("/getHUEindex", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{'status': 'success', 'lightsAmount': " + String(getHueLightIndex()) + "}");
  });

  server.on("/turnOffHUE", HTTP_GET, [](AsyncWebServerRequest *request) {
    int l = getHueLightIndex();
    for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, false); }

    request->send(200, "application/json", "{'status': 'success', 'message': 'OK'}");
  });

  server.on("/turnOnHUE", HTTP_GET, [](AsyncWebServerRequest *request) {
    int l = getHueLightIndex();
    for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, true); }

    request->send(200, "application/json", "{'status': 'success', 'message': 'OK'}");
  });

  // ----------------------------
  // Simple pages
  // ----------------------------

  // Error pages
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println(F("[T] WebServer: GET - 404."));
    request->send(404, "text/html", "<center><h1>Content not found.</center</h1>");
  });

  // Start the webserver
  server.begin();
  Serial.println("[T] WebServer: Start.");
}

#endif