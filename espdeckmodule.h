#ifndef ESPDECKMODULE
#define ESPDECKMODULE
class Module {
    public:
        String moduleName = "Unnamed module";
        String id = "unnamedModule";
        bool isEnabled = true;
        int matrixRefreshRate = 2000;
        int backendRefreshRate = 2000;

        virtual void begin() = 0; // funciton to mainly override parent class fields for info
        virtual void backend() = 0; // executing on core #0 for polling web updates asynchronously
        virtual void matrixProgram() = 0; // executing on main loop core #1
};
#endif