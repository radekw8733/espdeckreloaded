// Sample module to begin with

#ifndef SIMPLE_TEXT_DISPLAY_MODULE
#define SIMPLE_TEXT_DISPLAY_MODULE
class SimpleTextDisplay : public Module {
    private: 
        int counter;
        int hueCounter;
        char buffer[8];
    public:
        void begin() {
            moduleName = "Simple Text Display Module";
        }

        void backend() {
            // nothing to write
        }
        void matrixProgram() {
            counter += 1;
            Serial.println(counter);
            sprintf(buffer,"%d",counter);
            matrix.displayText(buffer,PA_CENTER,prefs.getInt("parola_speed",30),prefs.getInt("parola_pause",2000),PA_SCROLL_LEFT);
            ledStrip.clear();
            for (int i = 0; i < ledStrip.numPixels(); i++) {
                ledStrip.setPixelColor(i,ledStrip.ColorHSV(random(0,65535)));
                ledStrip.show();
            }
        }
};
#endif