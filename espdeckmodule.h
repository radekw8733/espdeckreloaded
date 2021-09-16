#ifndef ESPDECKMODULE
#define ESPDECKMODULE
class Module {
    public:
        String moduleName = "Unnamed module";
        int matrixRefreshRate = 2000;
        int backendRefreshRate = 2000;

        virtual void begin() = 0;
        virtual void backend() = 0;
        virtual void matrixProgram() = 0;
};
#endif