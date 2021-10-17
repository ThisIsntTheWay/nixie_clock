#include <Arduino.h>
#include <config.h>
#include <LITTLEFS.h>

// Declare statics
Configurator::NixieConfig Configurator::nixieConfiguration;
Configurator::RTCConfig Configurator::rtcConfiguration;
Configurator::NetConfig Configurator::netConfiguration;
Configurator::SystemConfig Configurator::sysConfiguration;

String Configurator::buildInfo = "";
String Configurator::fwInfo = "";

byte Configurator::sysStatus = 0;

// Main
void Configurator::prepareFS() {
    Configurator::FSReady = false;

    if (!LITTLEFS.begin()) {
        Serial.println("[X] FS: Filesystem mount failure.");
    } else {
        // Create configs
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");
            
        if (!LITTLEFS.exists("/config/rtcConfig.json")) {
            File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");

            // Construct JSON
            StaticJsonDocument<200> cfgRTC;

            const char* ntpSource = "ch.pool.ntp.org";

            cfgRTC["ntpSource"] = ntpSource;
            cfgRTC["isNTP"] = 1;
            cfgRTC["isDST"] = 1;
            cfgRTC["tzOffset"] = 3600;
            cfgRTC["manEpoch"] = 0;

            // Write rtcConfig.cfg
            if (!(serializeJson(cfgRTC, rtcConfig)))
                Serial.println(F("[X] RTC: Config write failure."));

            rtcConfig.close();
        }

        if (!LITTLEFS.exists("/config/nixieConfig.json")) {
            File configNixie = LITTLEFS.open(F("/config/nixieConfig.json"), "w");

            // Construct JSON
            StaticJsonDocument<200> cfgNixie;

            cfgNixie["brightness"] = 170;
            cfgNixie["depoisonMode"] = 1;
            cfgNixie["depoisonInterval"] = 60;

            // Write rtcConfig.cfg
            if (!(serializeJson(cfgNixie, configNixie)))
                Serial.println(F("[X] Nixie: Config write failure."));

            configNixie.close();
        }

        if (!LITTLEFS.exists("/config/netConfig.json")) {
            File configNetwork = LITTLEFS.open(F("/config/netConfig.json"), "w");

            // Construct JSON
            StaticJsonDocument<200> cfgNet;

            cfgNet["WiFiClient"] = 0;
            cfgNet["WiFi_SSID"] = "NaN";
            cfgNet["WiFi_PSK"] = "NaN";

            // Write rtcConfig.cfg
            if (!(serializeJson(cfgNet, configNetwork)))
                Serial.println(F("[X] Network: Config write failure."));

            configNetwork.close();
        }

        // Set FS ready flag
        Configurator::FSReady = true;
    }
}

void Configurator::parseRTCconfig() {
    if (FSReady) {
        // Read file
        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

        // Parse JSON
        StaticJsonDocument<250> cfgRTC;
        DeserializationError error = deserializeJson(cfgRTC, rtcConfig);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] RTC parser: Deserialization fault: "); Serial.println(err);
        } else {

            int ntpVal = cfgRTC["isNTP"];

            // Populate config struct
            strlcpy(rtcConfiguration.ntpSource, cfgRTC["ntpSource"], sizeof(rtcConfiguration.ntpSource));
            rtcConfiguration.isNTP = ntpVal;
            rtcConfiguration.isDST = cfgRTC["isDST"];
            rtcConfiguration.tzOffset = cfgRTC["tzOffset"];
            rtcConfiguration.manEpoch = cfgRTC["manEpoch"];
        }
        
        rtcConfig.close();
    }

    return;
}

void Configurator::parseNixieConfig() {
    if (FSReady) {
        // Read file
        File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "r");

        // Parse JSON
        StaticJsonDocument<250> cfgNixie;
        DeserializationError error = deserializeJson(cfgNixie, nixieConfig);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] Nixie parser: Deserialization fault: "); Serial.println(err);
        } else {
            // Populate config struct
            nixieConfiguration.brightness = cfgNixie["brightness"];
            nixieConfiguration.depoisonMode = cfgNixie["depoisonMode"];
            nixieConfiguration.depoisonInterval = cfgNixie["depoisonInterval"];
            
            nixieConfig.close();   
        }
    }
}


void Configurator::parseSysConfig() {
    if (FSReady) {
        // Read file
        File sysConfigFile = LITTLEFS.open(F("/config/sysConfig.json"), "r");

        // Parse JSON
        StaticJsonDocument<250> cfgSys;
        DeserializationError error = deserializeJson(cfgSys, sysConfigFile);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] Nixie parser: Deserialization fault: "); Serial.println(err);
        } else {
            // Populate config struct
            sysConfiguration.keepLEDpostBoot = cfgSys["keepLEDpostBoot"];
            
            sysConfigFile.close();   
        }
    }
}

void Configurator::parseNetConfig() {
    if (FSReady) {
        // Read file
        File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

        // Parse JSON
        DynamicJsonDocument cfgNet(2048);
        DeserializationError error = deserializeJson(cfgNet, netConfigF);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] Network parser: Deserialization fault: "); Serial.println(err);
        } else {
            // Populate config struct
            netConfiguration.WiFiClient = cfgNet["WiFiClient"];
            strlcpy(netConfiguration.WiFi_SSID, cfgNet["WiFi_SSID"], sizeof(netConfiguration.WiFi_SSID));
            strlcpy(netConfiguration.WiFi_PSK, cfgNet["WiFi_PSK"], sizeof(netConfiguration.WiFi_PSK));

            #ifdef DEBUG
                    Serial.print("Parsing WiFiClient as: ");
                    Serial.println(netConfiguration.WiFiClient);
                Serial.printf("[i] Network parser: SSID, PSK: '%s', '%s'\n", netConfiguration.WiFi_SSID, netConfiguration.WiFi_PSK);
            #endif

            netConfigF.close();   
        }
    }
}

void listFilesInDir(File dir, int numTabs) {
  while (true) {
 
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files in the folder
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFilesInDir(entry, numTabs + 1);
    } else {
      // display zise for file, nothing for directory
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void Configurator::nukeConfig() {
    // Destroy tasks
    vTaskDelete(task_perp_nix);
    vTaskDelete(task_perp_rtc);

    // Delete everything inside /config/ directory
    File dir = LITTLEFS.open("/config/");
    File f = dir.openNextFile();
    while (f) {
        Serial.print("[!] System: Destroying: ");
            Serial.println(f.name());
        LITTLEFS.remove(f.name());

        dir.openNextFile();
    }
}
