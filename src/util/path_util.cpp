#include "path_util.h"
#include <filesystem>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#include <pwd.h>
#endif

namespace nexora::util {

namespace fs = std::filesystem;

std::string PathUtil::GetExecutablePath() {
#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer);
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
#endif
}

std::string PathUtil::GetExecutableDir() {
    std::string exe_path = GetExecutablePath();
    if (exe_path.empty()) return ".";
    return fs::path(exe_path).parent_path().string();
}

std::string PathUtil::GetAppDataDir() {
#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        return Combine(std::string(path), "NEXORA");
    }
    return Combine(GetExecutableDir(), "AppData");
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (home) {
        return Combine(std::string(home), ".nexora");
    }
    return "./.nexora";
#endif
}

std::string PathUtil::GetUserDataDir() {
    std::string app_data = GetAppDataDir();
    std::string user_data = Combine(app_data, "User Data");
    EnsureDirectoryExists(user_data);
    return user_data;
}

std::string PathUtil::GetProfileDir(const std::string& profile_name) {
    std::string user_data = GetUserDataDir();
    std::string profile_dir = Combine(user_data, profile_name.empty() ? "Default" : profile_name);
    EnsureDirectoryExists(profile_dir);
    return profile_dir;
}

std::string PathUtil::GetCacheDir(const std::string& profile_name) {
    std::string profile_dir = GetProfileDir(profile_name);
    std::string cache_dir = Combine(profile_dir, "Cache");
    EnsureDirectoryExists(cache_dir);
    return cache_dir;
}

std::string PathUtil::GetLogDir() {
    std::string app_data = GetAppDataDir();
    std::string log_dir = Combine(app_data, "Logs");
    EnsureDirectoryExists(log_dir);
    return log_dir;
}

std::string PathUtil::GetDownloadsDir() {
#if defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
        std::string downloads = Combine(std::string(path), "Downloads");
        if (EnsureDirectoryExists(downloads)) {
            return downloads;
        }
    }
    return Combine(GetUserDataDir(), "Downloads");
#else
    const char* home = getenv("HOME");
    if (home) {
        std::string downloads = Combine(std::string(home), "Downloads");
        if (EnsureDirectoryExists(downloads)) {
            return downloads;
        }
    }
    return "./Downloads";
#endif
}

std::string PathUtil::GetParentDir(const std::string& path) {
    if (path.empty()) return ".";
    try {
        std::string parent = fs::path(path).parent_path().string();
        return parent.empty() ? "." : parent;
    } catch (...) {
        return ".";
    }
}

bool PathUtil::EnsureDirectoryExists(const std::string& path) {
    if (path.empty() || path == "." || path == "./") return true;
    try {
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool PathUtil::FileExists(const std::string& path) {
    if (path.empty()) return false;
    try {
        return fs::exists(path) && !fs::is_directory(path);
    } catch (...) {
        return false;
    }
}

bool PathUtil::DeleteFile(const std::string& path) {
    if (path.empty()) return false;
    try {
        if (fs::exists(path)) {
            return fs::remove(path);
        }
        return false;
    } catch (...) {
        return false;
    }
}

std::string PathUtil::Combine(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    return (fs::path(path1) / fs::path(path2)).string();
}

std::string PathUtil::NormalizePath(const std::string& path) {
    if (path.empty()) return "";
    try {
        return fs::path(path).lexically_normal().string();
    } catch (...) {
        return path;
    }
}

} // namespace nexora::util
