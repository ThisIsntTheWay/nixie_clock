#include <networkConfig.h>

String NetworkConfig::SSID = "";
String NetworkConfig::PSK = "";
bool NetworkConfig::IsAP = false;
bool NetworkConfig::InInit = true;
bool NetworkConfig::IsStatic = false;

/**************************************************************************/
/*!
    @brief Parses the network configuration file in the onboard Flash FS.
*/
/**************************************************************************/
bool NetworkConfig::parseNetConfig() {
    if (!LITTLEFS.exists(this->netFile)) {
        Serial.println(F("[i] NET: Creating net config."));

        File netConfig = LITTLEFS.open(this->netFile, "w");

        // Construct JSON
        StaticJsonDocument<200> cfgNET;

        cfgNET["ssid"] = "mySSID";
        cfgNET["psk"] = "myPSK";
        cfgNET["IsAP"] = true;

        // Write NetConfig.cfg
        if (!(serializeJson(cfgNET, netConfig))) {
            Serial.println(F("[X] NET: Config write failure."));
        }

        // Makse sure IsAP is actually true
        this->IsAP = true;

        netConfig.close();
        return true;
    } else {
        File netConfig = LITTLEFS.open(this->netFile, "r");
        
        // Parse JSON
        StaticJsonDocument<250> cfgNET;
        DeserializationError error = deserializeJson(cfgNET, netConfig);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] Net parser: Deserialization fault: "); Serial.println(err);
            netConfig.close();

            return false;
        } else {            
            JsonVariant jsonSSID = cfgNET["ssid"];
            JsonVariant jsonPSK = cfgNET["psk"];
            JsonVariant jsonAP = cfgNET["IsAP"];

            JsonVariant jsonIsStatic = cfgNET["isStatic"];
            JsonVariant jsonDeviceIP = cfgNET["deviceIP"];
            JsonVariant jsonNetmask = cfgNET["netmask"];
            JsonVariant jsonGateway = cfgNET["gateway"];
            JsonVariant jsonDNS1 = cfgNET["dns"];

            this->SSID = jsonSSID.as<String>();
            this->PSK = jsonPSK.as<String>();
            this->IsAP = jsonAP.as<bool>();

            // IP config
            bool t = jsonIsStatic.as<bool>();
            if (t) {
                this->isStatic = jsonIsStatic.as<bool>();
                this->deviceIP = jsonDeviceIP.as<const char*>();
                this->netmask = jsonNetmask.as<const char*>();
                this->gateway = jsonGateway.as<const char*>();
                this->dns1 = jsonDNS1.as<const char*>();
            }
        }

        return true;
    }
}

/**************************************************************************/
/*!
    @brief Returns the current IPv4 configuration.
    @param which Which IP parameter to return, from 0 to 4.
    @return Returns the requested IP as a string.
*/
/**************************************************************************/
String NetworkConfig::GetIPconfig(int8_t which) {
    switch (which) {
        case 0: return WiFi.localIP().toString(); break;
        case 1: return String(WiFi.subnetCIDR()); break;
        case 2: return WiFi.gatewayIP().toString(); break;
        case 3: return WiFi.dnsIP(0).toString(); break;
        case 4: return WiFi.macAddress(); break;
        default: Serial.printf("[X] Invalid GetIPconfig 'which': %d\n", which); throw;
    }
}

/**************************************************************************/
/*!
    @brief Writes IP configuration to netConfig.json.
    @param _refDoc Reference to the JSON document containing IP configuration data.
    @return Returns TRUE if no errors were encountered.
*/
/**************************************************************************/
bool NetworkConfig::WriteIPConfig(JsonDocument& _refDoc) {
    JsonVariant jsonDeviceIP = _refDoc["deviceIP"];
    JsonVariant jsonNetmask = _refDoc["netmask"];
    JsonVariant jsonGateway = _refDoc["gateway"];
    JsonVariant jsonDNS1 = _refDoc["dns"];

    File netConfig = LITTLEFS.open(this->netFile, "r");
    
    // Parse JSON
    StaticJsonDocument<250> cfgNET;
    DeserializationError error = deserializeJson(cfgNET, netConfig);
    if (error) {
        Serial.print("[X] netCFG deserialization fault: "); Serial.println(error.c_str());
        netConfig.close();

        return false;
    } else {
        // Reopen in writing mode.
        netConfig.close();
        File netConfig = LITTLEFS.open(this->netFile, "w");

        bool DHCP = _refDoc["useDHCP"];
        if (DHCP) {
            cfgNET["isStatic"] = (bool) false;
        } else {
            cfgNET["isStatic"] = (bool) true;
        }

        if (jsonDeviceIP) cfgNET["deviceIP"] = jsonDeviceIP.as<const char*>();
        if (jsonNetmask) cfgNET["netmask"] = jsonNetmask.as<const char*>();
        if (jsonGateway) cfgNET["gateway"] = jsonGateway.as<const char*>();
        if (jsonDNS1) cfgNET["dns"] = jsonDNS1.as<const char*>();
        
        serializeJsonPretty(cfgNET, Serial); Serial.println("");

        if (!(serializeJson(cfgNET, netConfig))) {
            Serial.println(F("[X] netCFG: Config write failure."));

            netConfig.close();
            return false;
        }

        netConfig.close();
        return true;
    }
    
    return true;
}

/**************************************************************************/
/*!
    @brief Applies the network configuration as defined in netConfig.json
*/
/**************************************************************************/
bool NetworkConfig::ApplyNetConfig() {
    File netConfig = LITTLEFS.open(this->netFile, "r");

    StaticJsonDocument<250> cfgNET;
    DeserializationError error = deserializeJson(cfgNET, netConfig);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] NET parser: Deserialization fault: "); Serial.println(err);
        netConfig.close();

        return false;
    } else {
        JsonVariant jsonIsStatic = cfgNET["isStatic"];
        JsonVariant jsonDeviceIP = cfgNET["deviceIP"];
        JsonVariant jsonNetmask = cfgNET["netmask"];
        JsonVariant jsonGateway = cfgNET["gateway"];
        JsonVariant jsonDNS1 = cfgNET["dns"];

        bool t = jsonIsStatic.as<bool>();
        if (t) {
            this->IsStatic = true;

            IPAddress deviceIP(0,0,0,0);
            IPAddress netmask(0,0,0,0);
            IPAddress gatewayIP(0,0,0,0);
            IPAddress dns1IP(0,0,0,0);

            if (jsonDeviceIP) {
                const char* tmp = jsonDeviceIP.as<const char*>();
                int devIP[4];

                // Save into IPAddress
                this->splitIPaddress((char*)tmp, devIP);
                for (int i = 0; i < 4; i++) {
                    deviceIP[i] = devIP[i];
                }
            }
            if (jsonNetmask) {
                const char* tmp = jsonNetmask.as<const char*>();
                int nMask[4];

                this->splitIPaddress((char*)tmp, nMask);
                for (int i = 0; i < 4; i++) {
                    netmask[i] = nMask[i];
                }
            }
            if (jsonGateway) {
                const char* tmp = jsonGateway.as<const char*>();
                int gate[4];

                this->splitIPaddress((char*)tmp, gate);
                for (int i = 0; i < 4; i++) {
                    gatewayIP[i] = gate[i];
                }
            }
            if (jsonDNS1) {
                const char* tmp = jsonDNS1.as<const char*>();
                int dns1[4];

                this->splitIPaddress((char*)tmp, dns1);
                for (int i = 0; i < 4; i++) {
                    dns1IP[i] = dns1[i];
                }
            }

            // Apply config
            /*if (jsonDeviceIP && jsonDNS1 && jsonGateway && jsonNetmask) { WiFi.config(deviceIP, dns1IP, gatewayIP, netmask); }
            else if (jsonDeviceIP && jsonDNS1 && jsonGateway)           { WiFi.config(deviceIP, dns1IP, gatewayIP); }
            else if (jsonDeviceIP && jsonDNS1)                          { WiFi.config(deviceIP, dns1IP); }
            else if (jsonDeviceIP)                                      { WiFi.config(deviceIP); }*/
            WiFi.config(deviceIP, gatewayIP, netmask, dns1IP);
        } else {
            this->IsStatic = false;
            Serial.println(F("[i] Will use DHCP."));
            
            netConfig.close();
            return false;
        }
        
        netConfig.close();
        return true;
    }
}

/**************************************************************************/
/*!
    @brief Split an IP Address (as char[]) and put each octet into an array.
    @param ingress IP to split, must have 4 octects.
    @param output Array to populate.
*/
/**************************************************************************/
bool NetworkConfig::splitIPaddress(char* ingress, int* output) {
    char* octetChar = strtok(ingress, ".");

    int tmp[4];
    int8_t i = 0; while (octetChar != NULL) {
        if (i > 3) {
            Serial.println("IP SPLIT: tmpIP out ouf range.");
            throw;
        }

        int octet = atoi(octetChar);
        tmp[i] = octet;
        i++;

        #ifdef DEBUG
            Serial.printf("> Octet: %d\n", octet);
        #endif DEBUG

        octetChar = strtok(NULL, ".");
    }

    // Copy into output
    memcpy(output, tmp, sizeof(tmp));

    return true;
}

/**************************************************************************/
/*!
    @brief Write to network configuration file.
    @param ssid SSID of network to connect to.
    @param psk PSK of network to connect to.
    @param IsAP TRUE = Schedule AP mode, FALSE = Schedule station mode.
*/
/**************************************************************************/
bool NetworkConfig::WriteWiFiConfig(const char* ssid, const char* psk, bool IsAP) {
    File netConfig = LITTLEFS.open(this->netFile, "w");
    StaticJsonDocument<200> cfgNET;

    cfgNET["ssid"] = ssid;
    cfgNET["psk"] = psk;
    cfgNET["IsAP"] = IsAP;

    // Write NetConfig.cfg
    if (!(serializeJson(cfgNET, netConfig))) {
        Serial.println(F("[X] NET: Config write failure."));

        netConfig.close();
        return false;
    }

    netConfig.close();
    return true;
}

/**************************************************************************/
/*!
    @brief (Overload 1) Write to network configuration file.
    @param ssid SSID of network to connect to.
    @param psk PSK of network to connect to.
*/
/**************************************************************************/
bool NetworkConfig::WriteWiFiConfig(const char* ssid, const char* psk) {
    File netConfig = LITTLEFS.open(this->netFile, "w");
    StaticJsonDocument<200> cfgNET;

    cfgNET["ssid"] = ssid;
    cfgNET["psk"] = psk;

    // Write NetConfig.cfg
    if (!(serializeJson(cfgNET, netConfig))) {
        Serial.println(F("[X] NET: Config write failure."));

        netConfig.close();
        return false;
    }

    netConfig.close();
    return true;
}

/**************************************************************************/
/*!
    @brief (Overload 2) Write to network configuration file.
    @param IsAP SSID of network to connect to.
*/
/**************************************************************************/
bool NetworkConfig::WriteWiFiConfig(bool IsAP) {
    File netConfig = LITTLEFS.open(this->netFile, "w");
    StaticJsonDocument<200> cfgNET;

    cfgNET["IsAP"] = IsAP;

    // Write NetConfig.cfg
    if (!(serializeJson(cfgNET, netConfig))) {
        Serial.println(F("[X] NET: Config write failure."));

        netConfig.close();
        return false;
    }

    netConfig.close();
    return true;
}

/**************************************************************************/
/*!
    @brief Initiates a WiFi connection depending on the network config JSON.
*/
/**************************************************************************/
void NetworkConfig::InitConnection() {
    WiFi.setHostname("nixie-clock");

    bool a = this->parseNetConfig();
    if (!a) {
        Serial.println("Could not parse net config.");
        this->IsAP = true;
    }

    // Determine if station or AP mode.
    if (this->IsAP) {
        this->initSoftAP();
    } else {
        Serial.print("Connecting to "); Serial.println(this->SSID);
        WiFi.mode(WIFI_STA);
        WiFi.begin(this->SSID.c_str(), this->PSK.c_str());

        // Attempt connection        
        uint8_t retryLimit = 20;
        bool isSuccess = false;
        wl_status_t state;

        for (int i = 0; i <= retryLimit; i++) {
            state = WiFi.status();

            if (i == retryLimit) {
                Serial.println("");
                Serial.println("[X] Connection timed out.");
                break;
            } else if (state == WL_CONNECT_FAILED) {
                Serial.println("");
                Serial.println("[X] Connection failed.");
                break;
            } else {
                if (state != WL_CONNECTED) {
                    Serial.print(".");
                } else {
                    isSuccess = true;
                    Serial.println("");
                    break;
                }
            }

            delay(500);
        }

        // AP fallback if connection timed out.
        if (isSuccess) {
            this->ApplyNetConfig();
            
            Serial.println(F("Connected! IP config: "));
            Serial.print("> IP: "); Serial.println(WiFi.localIP());
            Serial.print("> NM: "); Serial.println(WiFi.subnetMask());
            Serial.print("> GW: "); Serial.println(WiFi.gatewayIP());
        } else {
            this->IsAP = true;
            this->initSoftAP();
        }
    }

    this->InInit = false;
}

/**************************************************************************/
/*!
    @brief Initiates the soft AP mode.
*/
/**************************************************************************/
void NetworkConfig::initSoftAP() {
    if (WiFi.softAP(this->softAPssid, this->softAPpsk)) {
        Serial.print("[i] WiFi AP is ready. IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[i] WiFi AP failed.");
    }
}