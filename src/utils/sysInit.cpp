/*
    ESP32 Nixie Clock - System initialization
    (c) V. Klopfenstein, 2021

    Code for system and network initialization
*/

#include <utils/sysInit.h>

bool FlashFSready = false;
bool WiFiReady;
bool APmode = true;
bool APisFallback;

void SysInit::listFilesInDir(File dir, int numTabs) {
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


String SysInit::parseNetConfig(int mode) {
    // Read file
    File netCfgFile = LITTLEFS.open(F("/config/netConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<300> cfgNet;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgNet, netCfgFile);
    if (error)
        Serial.println(F("[X] NET_P: Could not deserialize JSON."));

    // Populate config struct
    strlcpy(netConfig.Mode, cfgNet["Mode"], sizeof(netConfig.Mode));
    strlcpy(netConfig.AP_SSID, cfgNet["AP_SSID"], sizeof(netConfig.AP_SSID));
    strlcpy(netConfig.AP_PSK, cfgNet["AP_PSK"], sizeof(netConfig.AP_PSK));
    strlcpy(netConfig.WiFi_SSID, cfgNet["WiFi_SSID"], sizeof(netConfig.WiFi_SSID));
    strlcpy(netConfig.WiFi_PSK, cfgNet["WiFi_PSK"], sizeof(netConfig.WiFi_PSK));
    
    netCfgFile.close();

    switch (mode) {
        case 1: return netConfig.Mode; break;
        case 2: return netConfig.AP_SSID; break;
        case 3: return netConfig.AP_PSK; break;
        case 4: return netConfig.WiFi_SSID; break;
        case 5: return netConfig.WiFi_PSK; break;
        default: return "[NET: unknown mode]";
    }

    return String();
}