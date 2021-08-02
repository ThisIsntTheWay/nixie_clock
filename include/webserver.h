#ifndef WEBHOST_H
#define WEBHOST_H
#include <ESPAsyncWebServer.h>

void webServerAPIs();
void webServerStaticContent();
void webServerInitWS();

String processor(const String& var) ;

void serveContent(AsyncWebServerRequest *request, String file, bool process);
void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void eventHandlerWS(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);

#endif