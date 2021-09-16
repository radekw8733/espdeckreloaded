#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define DEBUG_TO_SERIAL // print debug info to usb serial port
#define ADD_FASTLED  // include FastLED library to add 

Preferences preferences;
MD_Parola matrix = MD_Parola(MD_MAX72XX::FC16_HW,5,4);
// MD_Parola matrix = MD_Parola(MD_MAX72XX::FC16_HW, DATA_PIN, CLK_PIN, CS_PIN);  // use software SPI on matrix
WebServer server(80);

#pragma region VARIOUS DECLARATIONS
String accessToken;
#pragma endregion

#pragma region MODULES
#include "espdeckmodule.h"
#include "modules/simpleTextDisplay.h"
SimpleTextDisplay simpleText;
std::vector<Module*> modules = {};
#pragma endregion

#include "artwork.h"
#ifdef ADD_FASTLED
#include <FastLED.h>
#endif

//MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,23,18,5,4);
void setup()
{
    #ifdef DEBUG_TO_SERIAL
    Serial.begin(115200);
    #endif
    for (int i = 0; i < 8; i++) {
        Serial.println(artwork[i]);
    }
    preferences.begin("espdeckreloaded");
    Serial.println("Initializing matrix screen...");
    matrix.begin();
    matrix.displayText("Initializing",PA_LEFT,preferences.getInt("parola_speed"),preferences.getInt("parola_pause"),PA_SCROLL_LEFT,PA_SCROLL_DOWN);
    if (!preferences.getBool("isInitialized")) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Espdeck Reloaded");
        server.on("/setup", []() {
            if (server.arg("accessToken") != "") {
                preferences.putString("accessToken",server.arg("accessToken"));
                accessToken = server.arg("accessToken");
                server.send(200,"text/plain","OK Access Token set");
                server.close();
                preferences.putBool("isInitialized",true);
            }
        });
        server.begin();
    }
    Serial.println("Initializing modules...");
    Serial.println("Modules in array: " + String(modules.size()));
    modules.push_back(&simpleText);
    for (int i = 0; i < modules.size(); i++) {
        modules[i]->begin();
    }
    Serial.println("Starting task on second core...");
    xTaskCreatePinnedToCore(moduleLoop,"Modules backend",2048,NULL,1,new TaskHandle_t,0);

}

void loop()
{
    for (int i = 0; i < modules.size(); i++) {
        Serial.println("Executing module " + String(i) + " module name: " + modules[i]->moduleName);
        matrix.displayClear();

        long millisStart = millis();
        modules[i]->matrixProgram();
        long millisEnd = millis();
        while (millisStart + (1000 - (millisEnd - millisStart)) > millis()) {
            matrix.displayAnimate();
        }
    }
}

void moduleLoop(void * parameters) {
    for (;;) {
        for (int i = 0; i < modules.size(); i++) {
            long millisStart = millis();
            modules[i]->backend();
            long millisEnd = millis();
            Serial.println("Module " + modules[i]->moduleName + " execution time: " + (millisEnd - millisStart) + " ms");
            vTaskDelay(1);
        }
    }
}