#include <Arduino.h>
#include "AsyncJson.h"
#include <config.h>
#include <LITTLEFS.h>
#include <AsyncElegantOTA.h>
#include <rtc.h>
#include <nixies.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Configurator cfg;
Nixies nix;
RTC rtc1;

// Replace placeholders in HTML files
String processor(const String& var) {
  if (var == "TUBES_BRIGHTNESS")   { return String((cfg.nixieConfiguration.brightness * 100) / 255);}   // Current Time

  return String();
}

void serveContent(AsyncWebServerRequest *request, String file, bool process) {
  if (LITTLEFS.exists(file)) {
    Serial.print("[i] Webserver: Serving ");
        Serial.println(file);

    if (process)    {request->send(LITTLEFS, file, String(), false, processor);}
    else            {request->send(LITTLEFS, file, String(), false, NULL);}
    
  } else {
    Serial.print("[i] Webserver: Cannot serve ");
        Serial.println(file);
  }
}

// Websocket ingress message handler
void eventHandlerWS(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    //Serial.printf("[T] WS got message: %s\n", (char*)data);   

    // Decide what to send based on message
    if      (strcmp((char*)data, "ackError") == 0)            { client->text("System error acknowledged."); }
    else if (strcmp((char*)data, "getTime") == 0)             { client->text("SYS_TIME " + rtc1.getTimeText()); }
/*  else if (strcmp((char*)data, "getSysMsg") == 0)           { 
        String msg;
        switch (cfg.sysStatus) {
            case 0:
                msg = "System nominal";
                break;
            case 3:
                msg = "System error";
                break;
        }

        client->text("SYS_MSG " + msg); 
    } */
    else if (strcmp((char*)data, "getRTCMode") == 0)          {
        String msg;
        if (cfg.rtcConfiguration.isNTP == 0)    msg = "Manual";
        if (cfg.rtcConfiguration.isNTP == 1)    msg = "NTP";
        client->text("SYS_MODE " + msg); 
    }
    else if (strcmp((char*)data, "getNTPsource") == 0)        { String m = String(cfg.rtcConfiguration.ntpSource); client->text("SYS_NTP " + m); }
    else if (strcmp((char*)data, "getGMTval") == 0)           { client->text("SYS_GMT " + String(cfg.rtcConfiguration.tzOffset)); }
    else if (strcmp((char*)data, "getDSTval") == 0)           {
        String msg;
        if (cfg.rtcConfiguration.isDST == 0)    msg = "Inactive";
        if (cfg.rtcConfiguration.isDST == 1)    msg = "Active";
        client->text("SYS_DST " + msg);
    }
    else if (strcmp((char*)data, "getCryptoTicker") == 0)     { client->text("SYS_CRYPTO " + String(cfg.nixieConfiguration.cryptoAsset) + "/" + String(cfg.nixieConfiguration.cryptoQuote)); }
    else if (strcmp((char*)data, "getWIFIssid") == 0)         { client->text("SYS_SSID " + String(cfg.netConfiguration.WiFi_SSID)); }
    else if (strcmp((char*)data, "getWIFIrssi") == 0)         { client->text("SYS_RSSI " + String(WiFi.RSSI()) + "db"); }
  //else if (strcmp((char*)data, "getDepoisonTime") == 0)     { client->text("NIXIE_DEP_TIME " + parseNixieConfig(2)); }
    else if (strcmp((char*)data, "getDepoisonInt") == 0)      { client->text("NIXIE_DEP_INTERVAL " + String(cfg.nixieConfiguration.depoisonInterval)); }
    else if (strcmp((char*)data, "getTubesBrightness") == 0)  { client->text("NIXIE_BRIGHTNESS " + String((cfg.nixieConfiguration.brightness * 100) / 255)); }
    else if (strcmp((char*)data, "getNixieDisplay") == 0)     { client->text("NIXIE_DISPLAY " + String(nix.t1) + String(nix.t2) + " " + String(nix.t3) + String(nix.t4)); }
    else if (strcmp((char*)data, "getNixieMode") == 0)        {
      if (cfg.nixieConfiguration.crypto) { client->text("NIXIE_MODE Crypto"); }
      else if (cfg.nixieConfiguration.mode == 1 && !cfg.nixieConfiguration.tumble) { client->text("NIXIE_MODE Clock"); }
      else if (cfg.nixieConfiguration.mode != 1 && cfg.nixieConfiguration.tumble) { client->text("NIXIE_MODE Cycling..."); }
      else if (cfg.nixieConfiguration.mode != 1 && !cfg.nixieConfiguration.tumble && !cfg.nixieConfiguration.crypto) { client->text("NIXIE_MODE Manual"); } }
    else if (strcmp((char*)data, "getDepoisonMode") == 0)  {
      String msg;
      
      switch (cfg.nixieConfiguration.depoisonMode) {
        case 1: msg = "On hour change"; break;
        case 2: msg = "Interval"; break;
        default: msg = "Mode unknown"; break;
      }

      client->text("NIXIE_DEP_MODE " + msg); 
    } else { client->text("Request unknown."); }
  }
}

void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
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

void webServerInitWS() {
  ws.onEvent(onWSEvent);
  server.addHandler(&ws);
}

AsyncCallbackJsonWebHandler *nethandler = new AsyncCallbackJsonWebHandler("/api/network", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Construct JSON
    StaticJsonDocument<250> data;
    if (json.is<JsonArray>()) { data = json.as<JsonArray>(); }
    else if (json.is<JsonObject>()) { data = json.as<JsonObject>(); }

    // Save JSON response as variables
    const char* rSSID = data["wifi_ssid"];
    const char* rPSK = data["wifi_psk"];
    const char* mode = data["mode"];

    #ifdef DEBUG
        Serial.print("[i] NET_W: Got data: ");
            Serial.println(rSSID);
            Serial.println(rPSK);
            Serial.println(mode);
    #endif

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
            if (data["mode"] == "AP") {
                tmpJSON["WiFiClient"] = 0;
            } else if (data["mode"] == "Client") {
                tmpJSON["WiFiClient"] = 1;
            } else {
                InputValid = false;
                errMsg = errMsg + String(" Mode unknown.");
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
            
            netConfigF.close();
        } else {
            request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Config was updated - ESP rebooting...\"}");
            netConfigF.close();

            delay(2000);
            ESP.restart();
        }
    }

    Serial.println(response);
});

AsyncCallbackJsonWebHandler *rtchandler = new AsyncCallbackJsonWebHandler("/api/rtc", [](AsyncWebServerRequest *request, JsonVariant &json) {
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
        tmpJSON["isNTP"] = rM;

        // NTP mode
        int e = 0;
        if (data["mode"] == "ntp") {
            if (data.containsKey("value")) {
                if (String(rV).length() > 5) { tmpJSON["ntpSource"] = rV; }
                else { InputValid = false; errMsg = errMsg + String(" Server bad length."); }
            } else { e++; }
            
            if (data.containsKey("gmt")) {
                if (String(rGMT).length() < 5) { tmpJSON["tzOffset"] = rGMT; }
                else { InputValid = false; errMsg = errMsg + String(" GMT bad length."); }
            } else { e++; }

            // Set DST
            if (data.containsKey("dst")) {
                if ( String(rDST).equals(String("true")) ) {
                    tmpJSON["DST"] = 1;
                } else if ( String(rDST).equals(String("false")) ) {
                    tmpJSON["DST"] = 0;
                } else {
                    InputValid = false; errMsg = errMsg + String(" DST unknown value.");
                }
            } else { e++; } 
        }

        // Manual mode
        else if (data["mode"] == "manual") {
            tmpJSON["manEpoch"] = rV;
            tmpJSON["isNTP"] = 0;
            //rtc.adjust(rV);
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

AsyncCallbackJsonWebHandler *nixiehandler = new AsyncCallbackJsonWebHandler("/api/nixies", [](AsyncWebServerRequest *request, JsonVariant &json) {
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

    // Cache nixie config JSON
    cfg.parseNixieConfig();
    
    // Populate new numbers
    if (!data["nNum1"].isNull()) nNum1 = data["nNum1"];
    if (!data["nNum2"].isNull()) nNum2 = data["nNum2"];
    if (!data["nNum3"].isNull()) nNum3 = data["nNum3"];
    if (!data["nNum4"].isNull()) nNum4 = data["nNum4"];

    #ifdef DEBUG
    Serial.print("Manual is: "); Serial.println(manual);
    #endif

    if (!data["brightness"].isNull()) {
        brightness = data["brightness"];
    }

    // Mode switches
    if (!data["mode"].isNull()) {
    if (data["mode"] == "manual")         { manual = true; cfg.nixieConfiguration.crypto = false; }
    else if (data["mode"] == "clock")     { manual = false; cfg.nixieConfiguration.crypto = false; }
    else if (data["mode"] == "tumbler")   { manual = false; cfg.nixieConfiguration.crypto = false; cfg.nixieConfiguration.tumble = true; }
    else if (data["mode"] == "crypto")    { manual = true; cfg.nixieConfiguration.crypto = true; configUpdate = true; }
    else if (data["mode"] == "depoison")  {
        configUpdate = true;

        depMode = data["dep_mode"];
        depInterval = data["dep_interval"];

        Serial.print("depMode, depInterval: "); Serial.print(depMode); Serial.println(depInterval);
    }
    } else {
    // Preserve current modes
    if (cfg.nixieConfiguration.mode != 1 && !cfg.nixieConfiguration.crypto ) { manual = true; }
    else if (cfg.nixieConfiguration.crypto) { manual = true; }
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

            if (!data["crypto_asset"].isNull()) tmpJSON["cryptoAsset"] = crypto_asset; Serial.print("[i] Webserver: Writing crypto_asset: "); Serial.println(crypto_asset);
            if (!data["crypto_quote"].isNull()) tmpJSON["cryptoQuote"] = crypto_quote; Serial.print("[i] Webserver: Writing crypto_quote: "); Serial.println(crypto_quote);
        }

        // Depoison
        if (!data["dep_mode"].isNull()) {
            #ifdef DEBUG
                Serial.println("[i] REST: dep_mode specified.");
            #endif

            // Verify that depoison mode is valid
            if (depMode > 3 || depMode < 0) {
                request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Unknown depoison mode specified.\"}");
                InputValid = false;
            } else {
                tmpJSON["depoisonMode"] = depMode;
                Serial.println("[i] Webserver: Writing cathodeDepoisonMode");
            }
        }
    
        if (data.containsKey("dep_interval")) tmpJSON["depoisonInterval"] = depInterval;

        // Brightness
        if (brightness < 101 && !data["brightness"].isNull()) {
            int bright = (brightness * 255) / 100;
            Serial.print("Webserver: Got bright, setting as: ");
                Serial.println(bright);

            tmpJSON["brightness"] = bright;

        } else if (brightness > 100 && !data["brightness"].isNull()) {
            request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Brightness value is out of bounds.\"}");
            InputValid = false;
        }
    }

    // Write config.cfg
    if (InputValid) {
        File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "w");
        if (!(serializeJson(tmpJSON, nixieConfig))) {
            Serial.println(F("[X] WebServer: Config write failure."));
            request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"" + errMsg + "\"}");
        } else {        
            if (cfg.nixieConfiguration.crypto) {
                cfg.nixieConfiguration.mode = 3;
            }

            // Only send request if config was actually updated.
            if (configUpdate)
                request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixie config has been updated.\"}");
        }

        nixieConfig.close();
    }
    
    // Cache nixie config JSON
    cfg.parseNixieConfig();

    // Serialize JSON
    //Serial.print("Manual, cycleNixies, crypto: "); Serial.print(manual); Serial.print(cycleNixies); Serial.println(crypto);
    String response;
    serializeJson(data, response);
        if (!configUpdate) {
        // Handle conditions
        if (manual && !cfg.nixieConfiguration.tumble && !cfg.nixieConfiguration.crypto) {
            cfg.nixieConfiguration.mode = 2;
            cfg.nixieConfiguration.nNum1 = nNum1;
            cfg.nixieConfiguration.nNum2 = nNum2;
            cfg.nixieConfiguration.nNum3 = nNum3;
            cfg.nixieConfiguration.nNum4 = nNum4;

            request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies now in manual mode.\"}");

        } else if (cfg.nixieConfiguration.tumble && !cfg.nixieConfiguration.crypto) {
            cfg.nixieConfiguration.tumble = true;
            request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies are now cycling...\"}");
        } else if (!manual && !cfg.nixieConfiguration.crypto) {
            cfg.nixieConfiguration.mode = 1;
            request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies now in autonomous mode.\"}");
        /*} else if (cfg.nixieConfiguration.crypto)
            cfg.nixieConfiguration.mode = 3;
            request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Nixies now in crypto mode.\"}");*/
        } else {
            request->send(400, "application/json", "{\"status\": \"error\", \"message\": \"Unexpected data.\"}");
        } 
    }

    Serial.println(response);
});

// API calls
void webServerAPIs() {
    // Add handlers
    server.addHandler(rtchandler);
    server.addHandler(nixiehandler);
    server.addHandler(nethandler);

    // Status pages
    /*server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/html", "Error: 404");
    });*/

    // Serve content
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

            String RSSIlabel = "";
            if (WiFi.RSSI(i) < -70) { RSSIlabel = "Weak"; }
            else if (WiFi.RSSI(i) < -60) { RSSIlabel = "Fair"; }
            else if (WiFi.RSSI(i) < -50) { RSSIlabel = "Good"; }
            else if (WiFi.RSSI(i) > -50) { RSSIlabel = "Excellent"; }

            RSSIlabel += " (" + String(WiFi.RSSI(i)) + "db)";

            if (i) json += ",";
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
            json += ",\"rssi\":\"" + RSSIlabel + "\"";
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

    AsyncElegantOTA.begin(&server);
    server.begin();

    Serial.println("[i] Webserver: Starting.");
}

void webServerStaticContent() {
    // Pages
        // Root / Index
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/html/index.html";
            serveContent(request, f, false);
        });

        // Tube control
        server.on("/tube", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/html/cfgTUBE.html";
            serveContent(request, f, true);
        });

        // RTC control
        server.on("/rtc", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/html/cfgRTC.html";
            serveContent(request, f, false); 
        });

        // Philips HUE control
        server.on("/hue", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/html/cfgHUE.html";
            serveContent(request, f, false);
        });

        // Network config
        server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/html/cfgNET.html";
            serveContent(request, f, false);
        });
    
    // Resources
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

        // Global CSS
        server.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/css/style.css";
            request->send(LITTLEFS, f);
        });

        // Toast
        server.on("/css/toast.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            String f = "/css/toast.css";
            request->send(LITTLEFS, f);    
        });

        // Serve favicon
        server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LITTLEFS, "/html/favicon.png", "image/png");
        });

    // Debug
        #ifdef DEBUG
            server.on("/very/secret/destroy", HTTP_GET, [](AsyncWebServerRequest *request) {
                cfg.nukeConfig();
                request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Destroying configuration... The system will reboot.\"}");

                delay(1000);
                ESP.restart(); 
            });
        #endif
}
