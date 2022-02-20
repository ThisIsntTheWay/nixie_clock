#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#define EEPROM_AUTH_GEN_FLAG_ADDR 0
#define EEPROM_AUTH_VIEW_FLAG_ADDR 1
#define EEPROM_AUTHCODE_ADDR 2

#include <EEPROM.h>

class Authentication {
    public:
        String GetAuthCode();
        void Initialize();
        bool CanShowAuthCode();
        bool SetFlag();

    private:
        void generateAuthCode();
        void getAuthCode();

        byte authCodeGenerated;
        byte authCodeSeen;
        int authCodeSeed;
        char authCode[13];
};

#endif