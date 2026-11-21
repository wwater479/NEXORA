#include "engine/cef_app_handler.h"

#if defined(_WIN32)
#include <windows.h>

#if defined(NEXORA_ENABLE_CEF)
#include "include/cef_app.h"
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;

#if defined(NEXORA_ENABLE_CEF)
    CefMainArgs main_args(hInstance);
    CefRefPtr<nexora::engine::CefAppHandler> app_handler(new nexora::engine::CefAppHandler());
    return CefExecuteProcess(main_args, app_handler.get(), nullptr);
#else
    return 0;
#endif
}

#else

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return 0;
}

#endif
