#include "core/app_lifecycle.h"
#include "engine/cef_app_handler.h"
#include "util/logger.h"

#if defined(_WIN32)
#include <windows.h>

#if defined(NEXORA_ENABLE_CEF)
#include "include/cef_app.h"
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;

    int argc = 0;
    LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    std::vector<char*> argv_vec;

    for (int i = 0; i < argc; ++i) {
        int count = WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, nullptr, 0, nullptr, nullptr);
        std::string str(count - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, &str[0], count, nullptr, nullptr);
        args.push_back(str);
    }
    LocalFree(argv_w);

    for (auto& s : args) {
        argv_vec.push_back(&s[0]);
    }

#if defined(NEXORA_ENABLE_CEF)
    CefMainArgs main_args(hInstance);
    CefRefPtr<nexora::engine::CefAppHandler> app_handler(new nexora::engine::CefAppHandler());

    int exit_code = CefExecuteProcess(main_args, app_handler.get(), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefSettings settings;
    settings.multi_threaded_message_loop = true;
    
    // Check if --no-sandbox was explicitly requested for debug/testing
    bool no_sandbox_requested = false;
    for (const auto& a : args) {
        if (a == "--no-sandbox") no_sandbox_requested = true;
    }
    settings.no_sandbox = no_sandbox_requested;

    // Direct separate worker subprocesses (Renderers, GPU, Utility) to dedicated executable
    CefString(&settings.browser_subprocess_path).FromString("NEXORA_Subprocess.exe");

    CefInitialize(main_args, settings, app_handler.get(), nullptr);
#endif

    if (!nexora::core::AppLifecycle::Instance().Initialize(argc, argv_vec.data())) {
        return 1;
    }

    int ret = nexora::core::AppLifecycle::Instance().Run();
    nexora::core::AppLifecycle::Instance().Shutdown();

#if defined(NEXORA_ENABLE_CEF)
    CefShutdown();
#endif

    return ret;
}

#else

int main(int argc, char* argv[]) {
    if (!nexora::core::AppLifecycle::Instance().Initialize(argc, argv)) {
        return 1;
    }

    int ret = nexora::core::AppLifecycle::Instance().Run();
    nexora::core::AppLifecycle::Instance().Shutdown();
    return ret;
}

#endif
