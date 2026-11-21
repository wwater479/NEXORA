#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace nexora::core {

enum class ThemeMode {
    System = 0,
    Light = 1,
    Dark = 2
};

enum class StartupBehavior {
    RestoreSession = 0,
    OpenNewTab = 1,
    OpenSpecificUrls = 2
};

struct MemoryConfig {
    bool enable_memory_optimizer = true;
    int freeze_delay_seconds = 300;     // 5 minutes
    int discard_delay_seconds = 900;    // 15 minutes
    bool aggressive_discard_under_pressure = true;
    int moderate_pressure_threshold_percent = 80;
    int critical_pressure_threshold_percent = 90;
    int max_active_renderer_processes = 4;
    bool enable_timer_throttling = true;
    bool protect_pinned_tabs = true;
    bool protect_audio_tabs = true;
    std::vector<std::string> protected_domains;
};

struct DeveloperConfig {
    bool enable_developer_mode = true;
    int task_manager_refresh_interval_ms = 1000;
    bool enable_remote_debugging = false;
    int remote_debugging_port = 9222;
    bool show_memory_in_tab_title = false;
};

struct BrowserConfig {
    ThemeMode theme = ThemeMode::System;
    std::string search_engine_url = "https://www.google.com/search?q=";
    std::string search_engine_name = "Google";
    std::string home_page_url = "nexora://newtab";
    StartupBehavior startup_behavior = StartupBehavior::RestoreSession;
    std::vector<std::string> startup_urls;
    bool show_bookmark_bar = true;
    bool hardware_acceleration = true;
    std::string default_download_dir;

    MemoryConfig memory;
    DeveloperConfig devtools;
};

class ConfigManager {
public:
    static ConfigManager& Instance() {
        static ConfigManager instance;
        return instance;
    }

    bool Init(const std::string& config_file_path);
    bool Load();
    bool Save();

    BrowserConfig GetConfig() const;
    void UpdateConfig(const BrowserConfig& config);

    MemoryConfig GetMemoryConfig() const;
    void UpdateMemoryConfig(const MemoryConfig& memory_config);

    DeveloperConfig GetDeveloperConfig() const;
    void UpdateDeveloperConfig(const DeveloperConfig& dev_config);

    ThemeMode GetTheme() const;
    void SetTheme(ThemeMode theme);

    bool IsDomainProtected(const std::string& domain) const;
    void AddProtectedDomain(const std::string& domain);
    void RemoveProtectedDomain(const std::string& domain);

private:
    ConfigManager();
    ~ConfigManager();

    std::string config_file_path_;
    mutable std::mutex mutex_;
    BrowserConfig config_;
};

} // namespace nexora::core
