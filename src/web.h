#ifndef WEB_H
#define WEB_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Define vars as used in web.ccp
// 'extern' makes them available in all namespaces
extern const char htmlOTA[] PROGMEM;
extern const char htmlRoot[] PROGMEM;
extern const char htmlRTCControl[] PROGMEM;
extern const char htmlTubeControl[] PROGMEM;
extern const char htmlPowerControl[] PROGMEM;

void handle_root(AsyncWebServerRequest *request);
void handle_OTA(AsyncWebServerRequest *request);
void handle_Settings(AsyncWebServerRequest *request);
void handle_Power(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);

#endif
