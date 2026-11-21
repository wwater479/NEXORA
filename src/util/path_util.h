#pragma once

#include <string>

namespace nexora::util {

class PathUtil {
public:
    static std::string GetExecutablePath();
    static std::string GetExecutableDir();
    static std::string GetAppDataDir();
    static std::string GetUserDataDir();
    static std::string GetProfileDir(const std::string& profile_name = "Default");
    static std::string GetCacheDir(const std::string& profile_name = "Default");
    static std::string GetLogDir();
    static std::string GetDownloadsDir();

    static bool EnsureDirectoryExists(const std::string& path);
    static bool FileExists(const std::string& path);
    static bool DeleteFile(const std::string& path);
    static std::string Combine(const std::string& path1, const std::string& path2);
    static std::string GetParentDir(const std::string& path);
    static std::string NormalizePath(const std::string& path);
};

} // namespace nexora::util
