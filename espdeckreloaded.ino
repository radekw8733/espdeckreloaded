#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
//#include "artwork.h"
Preferences preferences;
MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,5,4);
WebServer server(80);
#pragma region VARIOUS DECLARATIONS
struct t_settings {
    int parola_speed = 30;
    int parola_pause = 2000;
};
t_settings globalSettings;
String accessToken;
#pragma endregion

#pragma region MODULES
#include "espdeckmodule.h"
#include "modules/simpleTextDisplay.h"
SimpleTextDisplay simpleText;
std::vector<Module*> modules = {};
#pragma endregion
#include "artwork.h"

//MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,23,18,5,4);
void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < 8; i++) {
        Serial.println(artwork[i]);
    }
    preferences.begin("espdeckreloaded");
    loadSettings();
    Serial.println("Initializing matrix screen...");
    lcd.begin();
    lcd.displayText("Initializing",PA_LEFT,globalSettings.parola_speed,globalSettings.parola_pause,PA_SCROLL_LEFT,PA_SCROLL_DOWN);
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
        lcd.displayClear();

        long millisStart = millis();
        modules[i]->matrixProgram();
        long millisEnd = millis();
        while (millisStart + (1000 - (millisEnd - millisStart)) > millis()) {
            lcd.displayAnimate();
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

void loadSettings() {
    globalSettings.parola_speed = preferences.getInt("parola_speed", 30);
    globalSettings.parola_pause = preferences.getInt("parola_pause", 2000);
}
