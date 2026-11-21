#include "app_lifecycle.h"
#include "config_manager.h"
#include "profile_manager.h"
#include "window_manager.h"
#include "../memory/tab_lifecycle_manager.h"
#include "../memory/memory_pressure_controller.h"
#include "../devtools/task_manager.h"
#include "../devtools/devtools_manager.h"
#include "../storage/session_store.h"
#include "../util/path_util.h"
#include "../util/win32_util.h"
#include "../util/string_util.h"
#include "../util/logger.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nexora::core {

using namespace nexora::util;
using namespace nexora::memory;
using namespace nexora::devtools;

AppLifecycle::~AppLifecycle() {
    Shutdown();
}

AppOptions AppLifecycle::ParseCommandLine(int argc, char* argv[]) {
    AppOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (StringUtil::StartsWith(arg, "--profile=")) {
            opts.profile_name = arg.substr(10);
        } else if (arg == "--dev-mode" || arg == "-d") {
            opts.developer_mode = true;
        } else if (arg == "--no-sandbox") {
            opts.no_sandbox = true;
        } else if (StringUtil::StartsWith(arg, "--remote-debugging-port=")) {
            try {
                opts.remote_debugging_port = std::stoi(arg.substr(24));
            } catch (...) {}
        } else if (!StringUtil::StartsWith(arg, "--")) {
            opts.initial_urls.push_back(arg);
        }
    }
    return opts;
}

bool AppLifecycle::Initialize(int argc, char* argv[]) {
    if (is_initialized_) return true;

    options_ = ParseCommandLine(argc, argv);

    // 1. Setup Logger
    std::string log_file = PathUtil::Combine(PathUtil::GetLogDir(), "nexora.log");
    Logger::Instance().Init(log_file, LogLevel::Info);
    LOG_INFO << "=== Starting NEXORA Browser ===";

    // 2. High DPI awareness
    Win32Util::EnableDpiAwareness();

    // 3. Single instance check
    if (!Win32Util::AcquireSingleInstanceLock("Global\\NEXORA_SingleInstance_Mutex")) {
        LOG_INFO << "Another instance of NEXORA is already running. Forwarding request.";
        return false;
    }

    // 4. Configuration
    std::string config_path = PathUtil::Combine(PathUtil::GetUserDataDir(), "config.json");
    ConfigManager::Instance().Init(config_path);

    // 5. Profile Manager
    ProfileManager::Instance().Init(PathUtil::GetUserDataDir());
    if (!options_.profile_name.empty()) {
        ProfileManager::Instance().SwitchProfile(options_.profile_name);
    }

    // 6. Memory Optimization Engine
    auto mem_cfg = ConfigManager::Instance().GetMemoryConfig();
    MemoryPressureController::Instance().Init(
        mem_cfg.moderate_pressure_threshold_percent,
        mem_cfg.critical_pressure_threshold_percent);
    TabLifecycleManager::Instance().Init(
        mem_cfg.freeze_delay_seconds,
        mem_cfg.discard_delay_seconds);

    // 7. Developer tools & Task Manager
    auto dev_cfg = ConfigManager::Instance().GetDeveloperConfig();
    if (dev_cfg.enable_developer_mode || options_.developer_mode) {
        TaskManager::Instance().StartMonitoring(dev_cfg.task_manager_refresh_interval_ms);
        DevToolsManager::Instance().Init(
            dev_cfg.enable_remote_debugging || options_.remote_debugging_port > 0,
            options_.remote_debugging_port > 0 ? options_.remote_debugging_port : dev_cfg.remote_debugging_port);
    }

    // 8. Session handling / Initial window
    std::string session_path = PathUtil::Combine(PathUtil::GetProfileDir(), "session.json");
    storage::SessionStore session_store(session_path);
    storage::BrowserSessionData prev_session;

    bool restored = false;
    if (ConfigManager::Instance().GetConfig().startup_behavior == StartupBehavior::RestoreSession) {
        if (session_store.LoadSession(prev_session) && !prev_session.windows.empty()) {
            restored = WindowManager::Instance().RestoreSession(prev_session);
        }
    }

    if (!restored) {
        auto win = WindowManager::Instance().CreateWindow();
        if (!options_.initial_urls.empty()) {
            for (size_t i = 0; i < options_.initial_urls.size(); ++i) {
                if (i == 0) {
                    win->NavigateCurrentTab(options_.initial_urls[i]);
                } else {
                    int64_t tid = win->GetTabBar().CreateTab(options_.initial_urls[i], options_.initial_urls[i]);
                    memory::TabLifecycleManager::Instance().RegisterTab(tid, win->GetWindowId(), options_.initial_urls[i], options_.initial_urls[i]);
                }
            }
        }
    }

    is_initialized_ = true;
    is_running_ = true;
    LOG_INFO << "NEXORA initialized successfully";
    return true;
}

int AppLifecycle::Run() {
    if (!is_running_) return 1;

#if defined(_WIN32)
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
#else
    LOG_INFO << "Running headless / portable execution loop";
    return 0;
#endif
}

void AppLifecycle::Shutdown() {
    if (!is_initialized_) return;

    LOG_INFO << "Shutting down NEXORA...";

    // 1. Capture and save session
    std::string session_path = PathUtil::Combine(PathUtil::GetProfileDir(), "session.json");
    storage::SessionStore session_store(session_path);
    auto current_session = WindowManager::Instance().CaptureCurrentSession();
    current_session.crashed = false;
    session_store.SaveSession(current_session);

    // 2. Stop monitoring
    TaskManager::Instance().StopMonitoring();

    // 3. Close windows
    WindowManager::Instance().CloseAllWindows();

    // 4. Release single instance lock
    Win32Util::ReleaseSingleInstanceLock();

    is_running_ = false;
    is_initialized_ = false;
    LOG_INFO << "=== NEXORA Shutdown Complete ===";
    Logger::Instance().Shutdown();
}

} // namespace nexora::core
