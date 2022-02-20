#include <Arduino.h>
#include <displayController.h>
#include <websrv.h>
#include <terminalAux.h>
#include <timekeeper.h>
#include <LITTLEFS.h>
#include <networkConfig.h>
#include <EEPROM.h>

/* -----------------------
            VARS
   ----------------------- */
#define EEPROM_REGION 256
#define INTERACT_BUTTON_PIN 21
#define RESET_DELAY_MS 5000

TaskHandle_t taskDisplay;
TaskHandle_t taskOLed;
TaskHandle_t taskTime;

bool buttonState = true;
bool buttonIsBeingPressed = false;

/* -----------------------
            MAIN
   ----------------------- */

void setup() {
  Serial.begin(115200);

  DisplayController dController;
  xTaskCreate(taskSetStatusLED, "O_LED daemon", 4000, NULL, 4, &taskOLed);
  xTaskCreate(taskSetDisplay, "Display daemon", 6500, NULL, 5, &taskDisplay);
  xTaskCreate(taskVisIndicator, "TEMP", 3000, NULL, 1, NULL);
  dController.OnboardLEDmode = 3;

  // EEPROM must be initialized before LittleFS, otherwise it will fail.
  EEPROM.begin(EEPROM_REGION);

  if (!LITTLEFS.begin()) {
    Serial.println("[X] FS mount failure!");
  }
  
  // Network stuff
  NetworkConfig nConfig;
  nConfig.InitConnection();
  webServerInit();
  
  // Remaining tasks
  xTaskCreate(taskTimekeeper, "Time daemon", 4000, NULL, 3, &taskTime);
  dController.OnboardLEDmode = 0;

  // Interact button
  pinMode(INTERACT_BUTTON_PIN, INPUT_PULLUP);

  Serial.print(vt_green); Serial.println("System ready.");
  Serial.print(vt_default_colour);
}

void loop() {
  // Factory reset watchdog
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0L;

  buttonState = digitalRead(INTERACT_BUTTON_PIN);
  if (!buttonState) {
    if (!buttonIsBeingPressed) {
      buttonIsBeingPressed = true;

      previousMillis = millis();
    } else {
      // Watch how long the button was pressed for.
      if (currentMillis - previousMillis >= RESET_DELAY_MS) {
        Serial.println(F("[!] System is being reset..."));
        vTaskDelete(taskDisplay);
        vTaskDelete(taskOLed);
        vTaskDelete(taskTime);

        // Blank nixies, dim board LEDs
        Nixies n; n.BlankDisplay();
        ledcWrite(ONBOARD_LEDC_CHANNEL, 50);

        LITTLEFS.end();
        if (LITTLEFS.format()) {
          Serial.println(F(" > Formatting complete."));
        } else {
          Serial.println(F(" > Could not format."));
        }

        // Destroy EEPROM
        Serial.printf(" > Formatting %d bytes of EEPROM...\n", EEPROM_REGION);
        for (int i = 0 ; i < EEPROM_REGION; i++) {
          EEPROM.write(i, 0);
        }

        EEPROM.commit();

        Serial.println(F(" > Rebooting..."));
        ESP.restart();
      }
    }
  } else {
    buttonIsBeingPressed = false;
  }
}