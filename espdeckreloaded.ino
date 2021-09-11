#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;
MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,5,4);
//MD_Parola lcd = MD_Parola(MD_MAX72XX::FC16_HW,23,18,5,4);
void setup()
{
    preferences.begin("espdeckreloaded");
    lcd.begin();
    lcd.displayText("Initializing",PA_LEFT,30,2000,PA_SCROLL_LEFT,PA_SCROLL_DOWN);
    //lcd.print("Initializing");
    if (!preferences.getBool("isInitialized")) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Espdeck Reloaded");
    }
}

void loop()
{
    lcd.displayAnimate();
}
