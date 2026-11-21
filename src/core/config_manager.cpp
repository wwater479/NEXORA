#include "config_manager.h"
#include "../util/json_util.h"
#include "../util/path_util.h"
#include "../util/string_util.h"
#include "../util/logger.h"
#include <fstream>
#include <algorithm>

namespace nexora::core {

using namespace nexora::util;

ConfigManager::ConfigManager() {
    config_.default_download_dir = PathUtil::GetDownloadsDir();
    config_.memory.protected_domains = {"meet.google.com", "teams.microsoft.com", "zoom.us", "discord.com"};
}

ConfigManager::~ConfigManager() {
    Save();
}

bool ConfigManager::Init(const std::string& config_file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_file_path_ = config_file_path;
    return Load();
}

bool ConfigManager::Load() {
    if (config_file_path_.empty() || !PathUtil::FileExists(config_file_path_)) {
        return true;
    }

    std::ifstream file(config_file_path_);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JsonValue root = JsonValue::Parse(content);
    if (!root.IsObject()) return false;

    config_.theme = static_cast<ThemeMode>(root.Get("theme").AsInt(static_cast<int>(ThemeMode::System)));
    config_.search_engine_url = root.Get("search_engine_url").AsString("https://www.google.com/search?q=");
    config_.search_engine_name = root.Get("search_engine_name").AsString("Google");
    config_.home_page_url = root.Get("home_page_url").AsString("nexora://newtab");
    config_.startup_behavior = static_cast<StartupBehavior>(root.Get("startup_behavior").AsInt(static_cast<int>(StartupBehavior::RestoreSession)));
    config_.show_bookmark_bar = root.Get("show_bookmark_bar").AsBool(true);
    config_.hardware_acceleration = root.Get("hardware_acceleration").AsBool(true);
    config_.default_download_dir = root.Get("default_download_dir").AsString(PathUtil::GetDownloadsDir());

    JsonValue startup_urls = root.Get("startup_urls");
    if (startup_urls.IsArray()) {
        config_.startup_urls.clear();
        for (const auto& u : startup_urls.AsArray()) {
            config_.startup_urls.push_back(u.AsString());
        }
    }

    // Memory config
    JsonValue mem_obj = root.Get("memory");
    if (mem_obj.IsObject()) {
        config_.memory.enable_memory_optimizer = mem_obj.Get("enable_memory_optimizer").AsBool(true);
        config_.memory.freeze_delay_seconds = mem_obj.Get("freeze_delay_seconds").AsInt(300);
        config_.memory.discard_delay_seconds = mem_obj.Get("discard_delay_seconds").AsInt(900);
        config_.memory.aggressive_discard_under_pressure = mem_obj.Get("aggressive_discard_under_pressure").AsBool(true);
        config_.memory.moderate_pressure_threshold_percent = mem_obj.Get("moderate_pressure_threshold_percent").AsInt(80);
        config_.memory.critical_pressure_threshold_percent = mem_obj.Get("critical_pressure_threshold_percent").AsInt(90);
        config_.memory.max_active_renderer_processes = mem_obj.Get("max_active_renderer_processes").AsInt(4);
        config_.memory.enable_timer_throttling = mem_obj.Get("enable_timer_throttling").AsBool(true);
        config_.memory.protect_pinned_tabs = mem_obj.Get("protect_pinned_tabs").AsBool(true);
        config_.memory.protect_audio_tabs = mem_obj.Get("protect_audio_tabs").AsBool(true);

        JsonValue domains = mem_obj.Get("protected_domains");
        if (domains.IsArray()) {
            config_.memory.protected_domains.clear();
            for (const auto& d : domains.AsArray()) {
                config_.memory.protected_domains.push_back(d.AsString());
            }
        }
    }

    // Developer config
    JsonValue dev_obj = root.Get("devtools");
    if (dev_obj.IsObject()) {
        config_.devtools.enable_developer_mode = dev_obj.Get("enable_developer_mode").AsBool(true);
        config_.devtools.task_manager_refresh_interval_ms = dev_obj.Get("task_manager_refresh_interval_ms").AsInt(1000);
        config_.devtools.enable_remote_debugging = dev_obj.Get("enable_remote_debugging").AsBool(false);
        config_.devtools.remote_debugging_port = dev_obj.Get("remote_debugging_port").AsInt(9222);
        config_.devtools.show_memory_in_tab_title = dev_obj.Get("show_memory_in_tab_title").AsBool(false);
    }

    LOG_INFO << "Loaded configuration from " << config_file_path_;
    return true;
}

bool ConfigManager::Save() {
    if (config_file_path_.empty()) return false;

    JsonValue root(JsonValue::ObjectType{});
    root.Set("theme", static_cast<int>(config_.theme));
    root.Set("search_engine_url", config_.search_engine_url);
    root.Set("search_engine_name", config_.search_engine_name);
    root.Set("home_page_url", config_.home_page_url);
    root.Set("startup_behavior", static_cast<int>(config_.startup_behavior));
    root.Set("show_bookmark_bar", config_.show_bookmark_bar);
    root.Set("hardware_acceleration", config_.hardware_acceleration);
    root.Set("default_download_dir", config_.default_download_dir);

    JsonValue::ArrayType startup_arr;
    for (const auto& u : config_.startup_urls) {
        startup_arr.push_back(u);
    }
    root.Set("startup_urls", startup_arr);

    // Memory obj
    JsonValue mem_obj(JsonValue::ObjectType{});
    mem_obj.Set("enable_memory_optimizer", config_.memory.enable_memory_optimizer);
    mem_obj.Set("freeze_delay_seconds", config_.memory.freeze_delay_seconds);
    mem_obj.Set("discard_delay_seconds", config_.memory.discard_delay_seconds);
    mem_obj.Set("aggressive_discard_under_pressure", config_.memory.aggressive_discard_under_pressure);
    mem_obj.Set("moderate_pressure_threshold_percent", config_.memory.moderate_pressure_threshold_percent);
    mem_obj.Set("critical_pressure_threshold_percent", config_.memory.critical_pressure_threshold_percent);
    mem_obj.Set("max_active_renderer_processes", config_.memory.max_active_renderer_processes);
    mem_obj.Set("enable_timer_throttling", config_.memory.enable_timer_throttling);
    mem_obj.Set("protect_pinned_tabs", config_.memory.protect_pinned_tabs);
    mem_obj.Set("protect_audio_tabs", config_.memory.protect_audio_tabs);

    JsonValue::ArrayType domains_arr;
    for (const auto& d : config_.memory.protected_domains) {
        domains_arr.push_back(d);
    }
    mem_obj.Set("protected_domains", domains_arr);
    root.Set("memory", mem_obj);

    // Dev obj
    JsonValue dev_obj(JsonValue::ObjectType{});
    dev_obj.Set("enable_developer_mode", config_.devtools.enable_developer_mode);
    dev_obj.Set("task_manager_refresh_interval_ms", config_.devtools.task_manager_refresh_interval_ms);
    dev_obj.Set("enable_remote_debugging", config_.devtools.enable_remote_debugging);
    dev_obj.Set("remote_debugging_port", config_.devtools.remote_debugging_port);
    dev_obj.Set("show_memory_in_tab_title", config_.devtools.show_memory_in_tab_title);
    root.Set("devtools", dev_obj);

    PathUtil::EnsureDirectoryExists(PathUtil::GetParentDir(config_file_path_));
    std::ofstream file(config_file_path_);
    if (!file.is_open()) return false;

    file << root.ToString(true);
    return true;
}

BrowserConfig ConfigManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void ConfigManager::UpdateConfig(const BrowserConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    Save();
}

MemoryConfig ConfigManager::GetMemoryConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.memory;
}

void ConfigManager::UpdateMemoryConfig(const MemoryConfig& memory_config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.memory = memory_config;
    Save();
}

DeveloperConfig ConfigManager::GetDeveloperConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.devtools;
}

void ConfigManager::UpdateDeveloperConfig(const DeveloperConfig& dev_config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.devtools = dev_config;
    Save();
}

ThemeMode ConfigManager::GetTheme() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.theme;
}

void ConfigManager::SetTheme(ThemeMode theme) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.theme = theme;
    Save();
}

bool ConfigManager::IsDomainProtected(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string lower_domain = StringUtil::ToLower(domain);
    for (const auto& d : config_.memory.protected_domains) {
        if (StringUtil::EqualsIgnoreCase(d, lower_domain) ||
            StringUtil::EndsWith(lower_domain, "." + StringUtil::ToLower(d))) {
            return true;
        }
    }
    return false;
}

void ConfigManager::AddProtectedDomain(const std::string& domain) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (domain.empty()) return;
    for (const auto& d : config_.memory.protected_domains) {
        if (StringUtil::EqualsIgnoreCase(d, domain)) return;
    }
    config_.memory.protected_domains.push_back(domain);
    Save();
}

void ConfigManager::RemoveProtectedDomain(const std::string& domain) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(config_.memory.protected_domains.begin(), config_.memory.protected_domains.end(),
                             [&domain](const std::string& d) {
                                 return StringUtil::EqualsIgnoreCase(d, domain);
                             });
    if (it != config_.memory.protected_domains.end()) {
        config_.memory.protected_domains.erase(it, config_.memory.protected_domains.end());
        Save();
    }
}

} // namespace nexora::core
