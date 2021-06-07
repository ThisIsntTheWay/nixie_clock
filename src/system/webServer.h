/*
    ESP32 Nixie Clock - Webserver module
    (c) V. Klopfenstein, 2021

    This code block spawns an instance of ESPAsyncWebserver.
    Anything related to a webserver happens here.
*/

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

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"

#include <system/rtc.h>
#include <system/philipsHue.h>
#include <utils/utils.h>

#ifndef nixie_h
    #include <system/nixie.h>
#endif

// Create instances
AsyncWebServer server(80);
Nixie n;
RTCModule r;

//  ---------------------
//  FUNCTIONS
//  ---------------------

// Replace placeholders in HTML files
String processor(const String& var) {

  // Unfortunately, var can only be handled withlots of if conditions:
  // A switch..case statement cannot be used with datatype "string".
  if (var == "TIME" || var == "RTC_TIME")   { return r.getTime();           }   // Current Time
  else if (var == "NTP_SOURCE")             { return r.parseRTCconfig(1);   }   // Current NTP server
  else if (var == "TIME_MODE")              { return r.parseRTCconfig(2);   }   // Current time source
  else if (var == "GMT_VAL")                { return r.parseRTCconfig(3);   }   // Current GMT
  else if (var == "DST_VAL")                { return r.parseRTCconfig(4);   }   // Current DST
  else if (var == "HUE_BRIDGE")             { return parseHUEconfig(1);   }   // HUE bridge IP
  else if (var == "HUE_API_USER")           { return "* * *";             }   // HUE API User
  else if (var == "HUE_TOGGLEON_TIME")      { return parseHUEconfig(3);   }   // HUE toggle ON time return
  else if (var == "HUE_TOGGLEOFF_TIME")     { return parseHUEconfig(4);   }   // HUE toggle OFF time
  else if (var == "NET_MODE")               { return parseNetConfig(1);   }   // Connectivity mode
  else if (var == "AP_SSID")                { return parseNetConfig(2);   }   // ESP32 AP SSID
  else if (var == "AP_PSK")                 { return parseNetConfig(3);   }   // ESP32 AP PSK
  else if (var == "WIFI_SSID")              { return parseNetConfig(4);   }   // WiFi client SSID
  else if (var == "CRYPTO_TICKER")          { return n.parseNixieConfig(1); }   // Cryptocurrency ticker
  else if (var == "TUBES_BRIGHTNESS")       { return n.parseNixieConfig(6); } // Tube brightness
  else if (var == "WIFI_RSSI")              { return String(WiFi.RSSI()) + "db"; }   // WiFi network dbi/RSSI
  else if (var == "TUBES_DISPLAY")          { return String(tube1Digit) + "" + String(tube2Digit) + " " + String(tube3Digit) + "" + String(tube4Digit); }   // Nixie tubes display
  else if (var == "TUBES_MODE")             {                               // Nixie tubes mode
    if (nixieAutonomous && !cycleNixies) { return "Clock"; }
    else if (!nixieAutonomous && cycleNixies) { return "Cycling..."; }
    else if (!nixieAutonomous && !cycleNixies) { return "Manual"; }
    else if (crypto) { return "Crypto"; }
  }

  return String();
}

//  ---------------------
//  Websockets
//  ---------------------

AsyncWebSocket ws("/ws");

// Websocket ingress message handler
void eventHandlerWS(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    //Serial.printf("[T] WS got message: %s\n", (char*)data);   

    // Decide what to send based on message
    if      (strcmp((char*)data, "ackError") == 0)            { client->text("System error acknowledged."); globalErrorOverride = true; }
    else if (strcmp((char*)data, "getTime") == 0)             { client->text("SYS_TIME " + r.getTime()); }
    else if (strcmp((char*)data, "getSysMsg") == 0)           { client->text("SYS_MSG " + getSysMsg()); }
    else if (strcmp((char*)data, "getRTCMode") == 0)          { client->text("SYS_MODE " + r.parseRTCconfig(2)); }
    else if (strcmp((char*)data, "getNTPsource") == 0)        { client->text("SYS_NTP " + r.parseRTCconfig(1)); }
    else if (strcmp((char*)data, "getGMTval") == 0)           { client->text("SYS_GMT " + r.parseRTCconfig(3)); }
    else if (strcmp((char*)data, "getDSTval") == 0)           { client->text("SYS_DST " + r.parseRTCconfig(4)); }
    else if (strcmp((char*)data, "getHUEip") == 0)            { client->text("HUE_IP " + parseHUEconfig(1)); }
    else if (strcmp((char*)data, "getHUEon") == 0)            { client->text("HUE_ON_SCHED " + parseHUEconfig(3)); }
    else if (strcmp((char*)data, "getHUEoff") == 0)           { client->text("HUE_OFF_SCHED " + parseHUEconfig(4)); }
    else if (strcmp((char*)data, "getCryptoTicker") == 0)     { client->text("SYS_CRYPTO " + n.parseNixieConfig(1)); }
    else if (strcmp((char*)data, "getWIFIssid") == 0)         { client->text("SYS_SSID " + parseNetConfig(4)); }
    else if (strcmp((char*)data, "getWIFIrssi") == 0)         { client->text("SYS_RSSI " + String(WiFi.RSSI()) + "db"); }
    else if (strcmp((char*)data, "getDepoisonTime") == 0)     { client->text("NIXIE_DEP_TIME " + n.parseNixieConfig(2)); }
    else if (strcmp((char*)data, "getDepoisonInt") == 0)      { client->text("NIXIE_DEP_INTERVAL " + n.parseNixieConfig(4)); }
    else if (strcmp((char*)data, "getTubesBrightness") == 0)  { client->text("NIXIE_BRIGHTNESS " + n.parseNixieConfig(6)); 
    }
    else if (strcmp((char*)data, "getNixieDisplay") == 0)     { client->text("NIXIE_DISPLAY " + String(tube1Digit) + "" + String(tube2Digit) + " " + String(tube3Digit) + "" + String(tube4Digit)); }
    else if (strcmp((char*)data, "getNixieMode") == 0)        {
      if (crypto) { client->text("NIXIE_MODE Crypto"); }
      else if (nixieAutonomous && !cycleNixies) { client->text("NIXIE_MODE Clock"); }
      else if (!nixieAutonomous && cycleNixies) { client->text("NIXIE_MODE Cycling..."); }
      else if (!nixieAutonomous && !cycleNixies && !crypto) { client->text("NIXIE_MODE Manual"); } }
    else if (strcmp((char*)data, "getDepoisonMode") == 0)  {
      String msg;
      
      switch (n.nixieConfig.cathodeDepoisonMode) {
        case 1: msg = "On hour change"; break;
        case 2: msg = "Interval"; break;
        case 3: msg = "On schedule"; break;
        default: msg = "Mode unknown"; break;
      }

      client->text("NIXIE_DEP_MODE " + msg); 
    } else { client->text("Request unknown."); }
  }
}

// RTC WS endpoint
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[i] WS client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("[i] WS client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      //client->text("Message received.");
      eventHandlerWS(arg, data, len, client);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
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

  // Network config
  server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[T] WebServer: GET /network."));

    String f = "/html/cfgNET.html";

    if(LITTLEFS.exists(f)) {
      request->send(LITTLEFS, f, String(), false, processor);
    } else {
      Serial.println("[X] WebServer: GET /network - No local ressource.");
    }      
  });

  // ----------------------------
  // Supplementary
  // ----------------------------

  // Websockets JS
  server.on("/js/websockets.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    String f = "/js/websockets.js";
    request->send(LITTLEFS, f);    
  });
  
  // Global JS
  server.on("/js/global.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    String f = "/js/global.js";
    request->send(LITTLEFS, f);
  });

  // Global css
  server.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    String f = "/css/style.css";
    request->send(LITTLEFS, f);
  });
  
  // Debug interface
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    String f = "/html/debug.html";
    request->send(LITTLEFS, f, String(), false, processor);
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
  
  AsyncCallbackJsonWebHandler *rtchandler = new AsyncCallbackJsonWebHandler("/api/RTCendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
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
      Serial.println(F("[X] RTC_W: Could not deserialize JSON:"));
        Serial.println(error.c_str());
      request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Cannot deserialize JSON!\"}");
    } else {
      rtcConfig.close();

      String errMsg = String("Failure!");
      bool InputValid = true;

      // Write to file based on request body
      tmpJSON["Mode"] = rM;

      // NTP mode
      int e = 0;
      if (data["mode"] == "ntp") {
        if (data.containsKey("value")) {
          if (String(rV).length() > 5) { tmpJSON["NTP"] = rV; }
          else { InputValid = false; errMsg = errMsg + String(" Server bad length."); }
        } else { e++; }
        
        if (data.containsKey("gmt")) {
          if (String(rGMT).length() < 5) { tmpJSON["GMT"] = rGMT; }
          else { InputValid = false; errMsg = errMsg + String(" GMT bad length."); }
        } else { e++; }

        if (data.containsKey("dst")) {
          if (String(rDST).length() < 5) { tmpJSON["DST"] = rDST; }
          else { InputValid = false; errMsg = errMsg + String(" DST bad length."); }
        } else { e++; } 
      }

      // Manual mode
      else if (data["mode"] == "manual") {
        tmpJSON["manualTime"] = rV;
        rtc.adjust(rV);
      } else {
        InputValid = false;
        errMsg = errMsg + String(" No mode specified.");
      }

      // Handle empty request body
      if (e == 3) { InputValid = false; errMsg = errMsg + String(" Expected fields are empty.");}

      // Write rtcConfig.cfg
      File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");
      if (!(serializeJson(tmpJSON, rtcConfig)) || !InputValid) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"" + errMsg + "\"}");
      } else {
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Config was updated.\"}");
      }
    }

    rtcConfig.close();

    Serial.println(response);
  });
  server.addHandler(rtchandler);

  // ============
  // NIXIE ENDPOINT
  
  AsyncCallbackJsonWebHandler *nixiehandler = new AsyncCallbackJsonWebHandler("/api/nixieControl", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<325> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }
    
    // Save JSON response as variables
    // Update them if required
    int nNum1 = 10;
    int nNum2 = 10;
    int nNum3 = 10;
    int nNum4 = 10;
    int brightness = 123;

    bool manual = false;
    bool InputValid = true;
    bool configUpdate = false;

    int depMode;
    int depInterval;
    
    // Populate new numbers
    if (!data["nNum1"].isNull()) nNum1 = data["nNum1"];
    if (!data["nNum2"].isNull()) nNum2 = data["nNum2"];
    if (!data["nNum3"].isNull()) nNum3 = data["nNum3"];
    if (!data["nNum4"].isNull()) nNum4 = data["nNum4"];

    #ifdef DEBUG
      Serial.print("Manual is: "); Serial.println(manual);
    #endif

    if (!data["brightness"].isNull())
      brightness = data["brightness"];

    // Mode switches
    if (!data["mode"].isNull()) {
      if (data["mode"] == "manual")         { manual = true; crypto = false; }
      else if (data["mode"] == "clock")     { manual = false; crypto = false; }
      else if (data["mode"] == "tumbler")   { manual = false; crypto = false; cycleNixies = true; }
      else if (data["mode"] == "crypto")    { manual = true; crypto = true; configUpdate = true; }
      else if (data["mode"] == "depoison")  {
        configUpdate = true;

        depMode = data["dep_mode"];
        depInterval = data["dep_interval"];

        Serial.print("depMode, depInterval: "); Serial.print(depMode); Serial.println(depInterval);
      }
    } else {
      // Preserve current modes
      if (!nixieAutonomous && !crypto) { manual = true; }
      else if (crypto) { manual = true; }
    }

    // Read file
    StaticJsonDocument<250> tmpJSON;
    File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "r");

    String errMsg = "Failure!";

    DeserializationError error = deserializeJson(tmpJSON, nixieConfig);
    if (error) {
      String err = error.c_str();
      Serial.print(F("[X] NIXIE: Could not deserialize JSON:"));
        Serial.println(err);

      request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"System error: Cannot deserialize JSON: " + err + "\"}");
      nixieConfig.close();
    } else {
      nixieConfig.close();
      
      if (data["mode"] == "crypto") {
        const char* crypto_asset = data["crypto_asset"];
        const char* crypto_quote = data["crypto_quote"];

        if (!data["crypto_asset"].isNull()) tmpJSON["crypto_asset"] = crypto_asset; Serial.print("[i] Webserver: Writing crypto_asset: "); Serial.println(crypto_asset);
        if (!data["crypto_quote"].isNull()) tmpJSON["crypto_quote"] = crypto_quote; Serial.print("[i] Webserver: Writing crypto_quote: "); Serial.println(crypto_quote);
      }

      // Depoison
      if (!data["dep_mode"].isNull()) {
        Serial.println("Got depmode!");
        // Verify that depoison mode is valid
        if (depMode > 3 || depMode < 0) {
          request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Unknown depoison mode specified.\"}");
          InputValid = false;
        } else {
          depMode = tmpJSON["cathodeDepoisonMode"];
          Serial.println("[i] Webserver: Writing cathodeDepoisonMode");
        }
      }

      if (brightness < 101 && !data["brightness"].isNull()) {
        int bright = (brightness * 255) / 100;
        tmpJSON["anodePWM"] = bright;
      } else if (brightness > 100 && data["brightness"].isNull()) {
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Brightness value is out of bounds.\"}");
        InputValid = false;
      }
    
      if (data.containsKey("dep_interval")) depInterval = tmpJSON["cathodeDepoisonInterval"]; 
    }

    // Write config.cfg
    if (InputValid) {
      File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "w");
      if (!(serializeJson(tmpJSON, nixieConfig))) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"" + errMsg + "\"}");
      } else {
        if (crypto) nixieAutonomous = false;

        // Only send request if config was actually updated.
        if (configUpdate)
          request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixie config has been updated.\"}");
      }

      nixieConfig.close();
    }

    // Serialize JSON
    //Serial.print("Manual, cycleNixies, crypto: "); Serial.print(manual); Serial.print(cycleNixies); Serial.println(crypto);
    String response;
    serializeJson(data, response);
    if (!configUpdate) {
      // Handle conditions
      if (manual && !cycleNixies && !crypto) {
        nixieAutonomous = false;
        n.displayNumber(nNum1, nNum2, nNum3, nNum4);
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies now in manual mode.\"}");

      } else if (cycleNixies && !crypto) {
        nixieAutonomous = false;
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies are now cycling...\"}");

      } else if (!manual && !crypto) {
        nixieAutonomous = true;
        forceUpdate = true;
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies now in autonomous mode.\"}");

      } else {
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Unexpected data.\"}");

      } 
    }

    Serial.println(response);
  });
  server.addHandler(nixiehandler);

  // ============
  // HUE ENDPOINT
  
  AsyncCallbackJsonWebHandler *huehandler = new AsyncCallbackJsonWebHandler("/api/HUEendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }
    
    // Save JSON response as variables
    const char* rIP = data["bridgeip"];
    const char* rUSR = data["apiuser"];
    const char* rON = data["ontime"];
    const char* rOFF = data["offtime"];
    
    // Serialize JSON
    String response;
    serializeJson(data, response);

    // Read file
    StaticJsonDocument<200> tmpJSON;
    File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "r");

    DeserializationError error = deserializeJson(tmpJSON, hueConfig);
    if (error) {
      Serial.println(F("[X] HUE_W: Could not deserialize JSON:"));
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Cannot deserialize JSON!\"}");
    } else {
      hueConfig.close();

      String errMsg = String("Config write failure.");
      bool InputValid = true;

      // Validate entries and change if needed
      // Skip empty data fields
      int e = 0;
      if (data.containsKey("bridgeip")) {
        if (!(data["bridgeip"] == "NaN")) {
          if (String(rIP).length() > 7) { tmpJSON["IP"] = rIP; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rIP bad format. ");
          }
        }
      } else { e++; Serial.println("rIP validation failure.");}
      
      if (data.containsKey("apiuser")) {
        if (!(data["apiuser"] == "NaN")) {
          if (String(rUSR).length() > 7) { tmpJSON["user"] = rUSR; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rUSR bad format. ");
          }
        }
      } else { e++; Serial.println("rUSR validation failure.");}

      if (data.containsKey("ontime")) {
        if (!(data["ontime"] == "NaN")) {
          if (String(rON).length() < 5) { tmpJSON["toggleOnTime"] = rON; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rOn bad format. ");
          }
        }
      } else { e++; Serial.println("rOn validation failure.");}

      if (data.containsKey("offtime")) {
        if (!(data["offtime"] == "NaN")) {
          if (String(rON).length() < 5) { tmpJSON["toggleOffTime"] = rOFF; }
          else {
            InputValid = false;
            errMsg = errMsg + String(" rOff bad format.");
          }
        }
      } else { e++; Serial.println("rOff validation failure.");}

      // Write to file based on request body
      // Produce error if \"e\" is equal to 4, InputValid is false or JSON could not get serialized
      File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "w");
      if ( !(serializeJson(tmpJSON, hueConfig)) || !InputValid || (e == 4)) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"" + errMsg + "\"}");
      } else {
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Config was updated.\"}");
      }
    }

    hueConfig.close();

    Serial.println(response);
  });
  server.addHandler(huehandler);

  // ============
  // Network ENDPOINT
  
  AsyncCallbackJsonWebHandler *nethandler = new AsyncCallbackJsonWebHandler("/api/NETendpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }
    
    // Save JSON response as variables
    const char* rMode = data["mode"];
    const char* rSSID = data["wifi_ssid"];
    const char* rPSK = data["wifi_psk"];

    // Serialize JSON
    String response;
    serializeJson(data, response);

    // Read file
    StaticJsonDocument<500> tmpJSON;
    File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

    DeserializationError error = deserializeJson(tmpJSON, netConfigF);
    if (error) {
      Serial.println(F("[X] NET_W: Could not deserialize JSON:"));
      Serial.println(error.c_str());
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Cannot deserialize JSON!\"}");
        netConfigF.close();
    } else {
      netConfigF.close();

      String errMsg = String("Config write failure.");
      bool InputValid = true;

      // Validate entries and change if needed
      // Skip empty data fields
      if (data.containsKey("mode")) {
        if (data["mode"] == "AP" || data["mode"] == "Client") {
          tmpJSON["Mode"] = rMode;
        } else {
            InputValid = false;
            errMsg = errMsg + String(" Mode not recognized.");
        }
      }
      
      if (data.containsKey("wifi_ssid"))  tmpJSON["WiFi_SSID"] = rSSID;
      if (data.containsKey("wifi_psk"))   tmpJSON["WiFi_PSK"] = rPSK;

      // Write to file based on request body
      // Produce error if \"e\" is equal to 4, InputValid is false or JSON could not get serialized
      File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "w");
      if ( !(serializeJson(tmpJSON, netConfigF)) || !InputValid) {
        Serial.println(F("[X] WebServer: Config write failure."));
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"" + errMsg + "\"}");
      } else {
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Config was updated.\"}");
      }
    }

    netConfigF.close();

    Serial.println(response);
  });
  server.addHandler(nethandler);

  // ============
  // WiFi scanner

  server.on("/api/wifiScan", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[i] WebServer: Scanning for WiFi..."));
    String json = "[";

    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
      for (int i = 0; i < n; ++i) {
        String encryption;
        switch (WiFi.encryptionType(i)) {
          case 2: encryption = "WPA"; break;
          case 3: encryption = "WPA2"; break;
          case 4: encryption = "WPA2"; break;
          case 5: encryption = "WEP"; break;
          default: encryption = "Unknown"; break;
        }

        if (i) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"rssi\":\"" + String(WiFi.RSSI(i)) + "db\"";
        json += ",\"mac\":\"" + WiFi.BSSIDstr(i) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += ",\"security\":\"" + encryption + "\"";
        json += "}";
      }

      WiFi.scanDelete();

      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }

    json = json + "]";
    request->send(200, "application/json", json);
    json = String();
    
    Serial.println(F("[i] WebServer: WiFi scan done."));
  });

  server.on("/api/RTCsync", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (r.parseRTCconfig(2) == "ntp") {
      Serial.println("[T] WebServer: Enforcing RTC sync.");

      NTPClient timeClient(ntpUDP, r.rtcConfig.NTP, r.rtcConfig.GMT + r.rtcConfig.DST);
      timeClient.begin();
      
      if (timeClient.forceUpdate()) {
        NTPisValid = true;

        long ntpTime = timeClient.getEpochTime();
        rtc.adjust(DateTime(ntpTime));

        request->send(200, "application/json", "{\"status\": \"success\", \"epoch\": " + String(ntpTime) + "}");
      } else {
        request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"NTP server unresponsive.\"}");
      }

      timeClient.end();
    } else {
      request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"NTP is disabled.\"}");
    }
  });

  server.on("/api/turnOffHUE", HTTP_GET, [](AsyncWebServerRequest *request) {
    int l = getHueLightIndex();
    for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, false); }

    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"OK\"}");
  });

  server.on("/api/turnOnHUE", HTTP_GET, [](AsyncWebServerRequest *request) {
    int l = getHueLightIndex();
    for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, true); }

    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"OK\"}");
  });
  
  server.on("/debug/forceRestart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Rebooting...\"}");
    delay(1000);

    ESP.restart();
  });

  // ----------------------------
  // Simple pages
  // ----------------------------

  // Error pages
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println(F("[T] WebServer: GET - 404."));
    request->send(404, "text/html", "Error: 404");
  });

  // Start the webserver
  server.begin();
  Serial.println("[T] WebServer: Start.");
}

void taskSetupWebserver(void *parameter) {
  int i = 0;
  while (!WiFiReady) {
    /*if (i > 30) {
      Serial.println("[X] WebServer: Network timeout.");
      vTaskDelete(NULL);
    }*/

    i++;
    vTaskDelay(500);
  }

  // Boot server and websockets
  webServerStartup();
  initWebSocket();

  vTaskDelete(NULL);
}

#endif