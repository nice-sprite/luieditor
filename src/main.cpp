
#include "main.h"
#include "application/Application.h"
#include <tracy/Tracy.hpp>

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PWSTR pCmdLine,
                    int nCmdShow) 
{

    CoInitialize(0);
    AppState state;
    app_create(&state, hInstance, "Equinox");
    app_message_loop(&state);
    app_shutdown(&state);

    return 0;
}
