#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
Preferences preferences;
MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,5,4);
WebServer server(80);
#pragma region VARIOUS DECLARATIONS
struct t_settings {
    int parola_speed = 30;
};
String accessToken;
#pragma endregion
t_settings globalSettings;
//MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,23,18,5,4);
void setup()
{
    preferences.begin("espdeckreloaded");
    loadSettings();
    lcd.begin();
    lcd.displayText("Initializing",PA_LEFT,globalSettings.parola_speed,2000,PA_SCROLL_LEFT,PA_SCROLL_DOWN);
    //lcd.print("Initializing");
    if (!preferences.getBool("isInitialized")) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Espdeck Reloaded");
        server.on("/setup", []() {
            if (server.arg("accessToken") != "") {
                preferences.putString("accessToken",server.arg("accessToken"));
                accessToken = server.arg("accessToken");
                server.send(200,"text/plain","OK Access Token set");
            }
        });
        server.begin();
    }
}

void loop()
{
    lcd.displayAnimate();
}

void loadSettings() {
    globalSettings.parola_speed = preferences.getInt("parola_speed", 30);
}
