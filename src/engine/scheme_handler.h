#pragma once

#include <string>

namespace nexora::engine {

class SchemeHandler {
public:
    static std::string GetInternalPageHtml(const std::string& url);
    static bool IsInternalScheme(const std::string& url);

private:
    static std::string GenerateNewTabPageHtml();
    static std::string GenerateSettingsHtml();
    static std::string GenerateHistoryHtml();
    static std::string GenerateDownloadsHtml();
    static std::string GenerateTaskManagerHtml();
    static std::string GenerateMemoryHtml();
    static std::string GenerateVersionHtml();
    static std::string GenerateErrorPageHtml(const std::string& url, int error_code, const std::string& error_text);
};

} // namespace nexora::engine
