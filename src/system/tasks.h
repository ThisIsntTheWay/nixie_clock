/*
    ESP32 Nixie Clock - Tasks module
    (c) V. Klopfenstein, 2021

    Headers for tasks.cpp
*/

#ifndef tasks_h
#define tasks_h

#include <system/nixie.h>
#include <system/rtc.h>

#define FACT_RST 17

bool EnforceFactoryReset = false;

TaskHandle_t TaskRTC_Handle;
TaskHandle_t TaskNixie_Handle;
TaskHandle_t TaskBright_Handle;
TaskHandle_t TaskHUE_Handle;

void taskSetupNixie(void*);
void taskUpdateNixie(void*);
void taskUpdateNixieBrightness(void*);

void taskfactoryResetWDT(void*);

void taskSetupRTC(void*);
void taskUpdateRTC(void*);

void taskWiFi(void*);
void taskFSMount(void*);

#endif