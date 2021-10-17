#ifndef TASKS_H
#define TASKS_H
#include <config.h>

void taskSysInit(void* parameter);
void taskMonitorStatus(void* parameter);
void taskShowStatus(void* parameter);

void taskSetupWebserver(void* parameter);

void taskSetupNetwork(void* paramter);
void taskSetupNixies(void* paramter);
void taskSetupRTC(void* parameter);

void taskUpdateNixies(void* parameter);
void taskUpdateRTC(void* parameter);
void taskUpdateCaches(void* parameter);
void taskUpdateBrightness(void* parameter);
void taskUpdateDepoisonState(void* parameter);

#endif