#ifndef NETWORK_H
#define NETWORK_H
#include <Arduino.h>

class Network {
    public:
        void initialize();
        bool joinNetwork(char* SSID, char* PSK);

        struct NetConfig {
            byte Nmask;
        };
};

#endif