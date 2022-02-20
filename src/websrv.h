#ifndef WEBSRV_H
#define WEBSRV_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <LITTLEFS.h>
#include <displayController.h>
#include <timekeeper.h>

void webServerAPIs();
void webServerStaticContent();
void webServerInit();

#endif