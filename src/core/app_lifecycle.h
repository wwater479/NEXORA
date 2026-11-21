#pragma once

#include <string>
#include <vector>

namespace nexora::core {

struct AppOptions {
    std::string profile_name = "Default";
    bool developer_mode = false;
    bool no_sandbox = false;
    int remote_debugging_port = 0;
    std::vector<std::string> initial_urls;
};

class AppLifecycle {
public:
    static AppLifecycle& Instance() {
        static AppLifecycle instance;
        return instance;
    }

    bool Initialize(int argc, char* argv[]);
    int Run();
    void Shutdown();

    const AppOptions& GetOptions() const { return options_; }
    bool IsRunning() const { return is_running_; }

private:
    AppLifecycle() = default;
    ~AppLifecycle();

    AppOptions ParseCommandLine(int argc, char* argv[]);

    AppOptions options_;
    bool is_running_ = false;
    bool is_initialized_ = false;
};

} // namespace nexora::core
