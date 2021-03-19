#include "Arduino.h"
#include "WiFi.h"
#include "esp32-hal-cpu.h"
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define SSID    "Alter Eggstutz"
#define PSK     "Fischer1"

//  ---------------------
//  FUNCTIONS
//  ---------------------
void toggleLED(void * parameter){
  for(;;){

    // Pause the task again for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // Destroy task
    vTaskDelete(NULL);
  }
}

//  ---------------------
//  MAIN
//  ---------------------

void setup() {

    Serial.begin(115200);
    
    // Downgrade CPU clock
    setCpuFrequencyMhz(160);
    Serial.println(getCpuFrequencyMhz());
    Serial.println(F("DONE BOOTUP"));

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
    Serial.println("Attempting to establish a  WiFi connection!");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }

    Serial.println(F("Connected successfully."));
    Serial.print("IP: ");
        Serial.println(WiFi.localIP());

    // FreeRTOS task creation
    xTaskCreate(
        toggleLED,      // Function that should be called
        "Toggle LED",   // Name of the task (for debugging)
        1000,           // Stack size (bytes)
        NULL,           // Parameter to pass
        1,              // Task priority
        NULL           // Task handle
        //0               // CPU core
    );
}

// Stays empty as we have built an RTOS infrastructure
void loop() {}