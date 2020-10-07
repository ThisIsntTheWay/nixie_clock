#ifndef WEB_H
#define WEB_H

void handle_root(AsyncWebServerRequest *request);
void handle_OTA(AsyncWebServerRequest *request);
void handle_Settings(AsyncWebServerRequest *request);
void handle_Power(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);

#endif
