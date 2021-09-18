#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define DEBUG_TO_SERIAL // print debug info to usb serial port
#define ADD_NEOPIXEL  // include FastLED library to add 

Preferences prefs;
MD_Parola matrix = MD_Parola(MD_MAX72XX::FC16_HW,5,4);
// MD_Parola matrix = MD_Parola(MD_MAX72XX::FC16_HW, DATA_PIN, CLK_PIN, CS_PIN);  // use software SPI on matrix
WebServer server(80);
WebServer apiServer(80);

#pragma region VARIOUS DECLARATIONS
String accessToken;
bool wasResetHoldBefore = false;
long resetButtonTimeCounter;
#include "artwork.h"
#ifdef ADD_NEOPIXEL
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel ledStrip;
#endif
#pragma endregion

#pragma region MODULES
#include "espdeckmodule.h"
#include "modules/simpleTextDisplay.h"
SimpleTextDisplay simpleText;
std::vector<Module*> modules = {};
#pragma endregion

//MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,23,18,5,4);
void setup()
{
    #ifdef DEBUG_TO_SERIAL
    Serial.begin(115200);
    #endif
    for (int i = 0; i < 8; i++) {
        Serial.println(artwork[i]);
    }
    prefs.begin("espdeckreloaded");
    pinMode(14,INPUT_PULLUP);
    Serial.println("Initializing matrix screen...");
    matrix.begin();
    matrix.displayText("Initializing",PA_LEFT,prefs.getInt("parola_speed"),prefs.getInt("parola_pause"),PA_SCROLL_LEFT,PA_SCROLL_DOWN);
    #ifdef ADD_NEOPIXEL
    ledStrip = *(new Adafruit_NeoPixel(10, prefs.getInt("neopixel_pin",12)));
    ledStrip.begin();
    ledStrip.setBrightness(50);
    #endif
    if (!prefs.getBool("isInitialized")) {
        Serial.println("--- EspDeck not initialized ---");
        Serial.println("Setting up access point...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Espdeck Reloaded");
        server.on("/setup", []() {
            if (server.arg("accessToken") != "" && server.arg("wifiSSID") != "" && server.arg("wifiPASS") != "") {
                prefs.putString("accessToken",server.arg("accessToken"));
                prefs.putString("wifi_SSID",server.arg("wifiSSID"));
                prefs.putString("wifi_PASS",server.arg("wifiPASS"));
                if (server.arg("staticIP") == "true") {
                    prefs.putUInt("wifi_staticIp",server.arg("wifi_staticIp").toInt());
                    prefs.putUInt("wifi_gatewayIp",server.arg("wifi_gatewayIp").toInt());
                    prefs.putUInt("wifi_mask",server.arg("wifi_mask").toInt());
                }
                accessToken = server.arg("accessToken");
                server.send(200,"text/plain","OK Access Token set");
                server.close();
                prefs.putBool("isInitialized",true);
                ESP.restart();
            }
            else {
                server.send(400,"text/plain","No Access Token provided");
            }
        });
        server.begin();
        Serial.println("Waiting for connections");
        for (;;) {
            if (matrix.displayAnimate()) {
                matrix.displayClear();
                matrix.displayText("Waiting for setup. Go to app and configure your clock",PA_CENTER,50,1000,PA_SCROLL_LEFT);
            }
            server.handleClient();
        }
    }
    else {
        Serial.println("Connecting to wifi " + prefs.getString("wifi_SSID","") + " ...");
        WiFi.mode(WIFI_STA);
        if (prefs.getBool("wifi_isStatipIpSet")) {
            WiFi.config(
                IPAddress(prefs.getUInt("wifi_staticIp")),
                IPAddress(prefs.getUInt("wifi_gatewayIp")),
                IPAddress(prefs.getUInt("wifi_mask"))
            );
        }
        char wifiSSID[48];
        char wifiPASS[48];
        prefs.getString("wifi_SSID","").toCharArray(wifiSSID,sizeof(wifiSSID));
        prefs.getString("wifi_PASS","").toCharArray(wifiPASS,sizeof(wifiPASS));
        WiFi.begin(wifiSSID,wifiPASS);
        int retryCounter = 0;
        while (WiFi.status() != WL_CONNECTED) {
            retryCounter += 1;
            Serial.println(".");
            if (retryCounter > 20) {
                if (matrix.displayAnimate()) {
                    matrix.displayClear();
                    matrix.displayText("Still connecting... reset if you entered wrong wifi info",PA_CENTER,50,1000,PA_SCROLL_LEFT);
                }
                checkResetButton();
                delay(50);
            }
            else {
                delay(500);
            }
        }
        Serial.println("CONNECTED");
        Serial.println("ip: " + WiFi.localIP());
        apiServer.on("/toggleModule", [] {
            if (apiServer.arg("moduleID") != "") {
                for (int i = 0; i < modules.size(); i++) {
                    if (modules[i]->id == apiServer.arg("moduleID")) {
                        modules[i]->isEnabled = !modules[i]->isEnabled;
                        server.send(200,"text/plain","Module " + modules[i]->moduleName + " is now " + modules[i]->isEnabled ? "enabled" : "disabled");
                        return;
                    }
                }
                server.send(404,"text/plain","No module found");
            }
            else {
                server.send(400,"text/plain","No module id provided");
            }
        });
        apiServer.begin();
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
        if (modules[i]->isEnabled == true) {
            Serial.println("Executing module " + String(i) + " module name: " + modules[i]->moduleName);
            matrix.displayClear();
            long millisStart = millis();
            modules[i]->matrixProgram();
            long millisEnd = millis();
            while (millisStart + (modules[i]->matrixRefreshRate - (millisEnd - millisStart)) > millis()) {
                checkResetButton();
                matrix.displayAnimate();
                apiServer.handleClient();
            }
        }
    }
}

void checkResetButton() {
    if (digitalRead(14) == LOW) {
        if (wasResetHoldBefore == true) {
            if (millis() > (resetButtonTimeCounter + 3000)) {
                prefs.clear();
                ESP.restart();
            }
        }
        else {
            Serial.println("Button state change detected!");
            resetButtonTimeCounter = millis();
            wasResetHoldBefore = true;
        }
    }
    else {
        wasResetHoldBefore = false;
    }
}

void moduleLoop(void * parameters) {
    for (;;) {
        for (int i = 0; i < modules.size(); i++) {
            if (modules[i]->isEnabled == true) {
                long millisStart = millis();
                modules[i]->backend();
                long millisEnd = millis();
                while (millisStart + (modules[i]->backendRefreshRate - (millisEnd - millisStart)) > millis()) {
                    vTaskDelay(1);
                }
            }
        }
    }
}