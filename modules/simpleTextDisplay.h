#ifndef SIMPLE_TEXT_DISPLAY_MODULE
#define SIMPLE_TEXT_DISPLAY_MODULE
class SimpleTextDisplay : public Module {
    private: 
        int counter;
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
            lcd.displayText(buffer,PA_CENTER,globalSettings.parola_speed,globalSettings.parola_pause,PA_SCROLL_LEFT);
        }
};
#endif