#include <FS.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#ifndef webServer
#define webServer

// Request handling
AsyncWebServer server(80);

void webServerRequestHandler(){
  Serial.println(F("[T] webServerRequestHandler start"));
    // Root / Index
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/htmlRoot.htm");
    });
    
    // Tube control
    server.on("/tube", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/htmlTubes.htm");
    });

    // RTC control
    server.on("/rtc", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/htmlRTC.htm");
    });

    // Philips HUE control
    server.on("/hue", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/htmlHUE.htm");
    });

    server.begin();
}

#endif