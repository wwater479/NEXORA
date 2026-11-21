#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nexora::util {

class StringUtil {
public:
    static std::wstring Utf8ToUtf16(const std::string& utf8_str);
    static std::string Utf16ToUtf8(const std::wstring& utf16_str);

    static std::string Trim(const std::string& str);
    static std::string ToLower(const std::string& str);
    static std::string ToUpper(const std::string& str);

    static bool StartsWith(const std::string& str, const std::string& prefix, bool case_sensitive = true);
    static bool EndsWith(const std::string& str, const std::string& suffix, bool case_sensitive = true);
    static bool EqualsIgnoreCase(const std::string& a, const std::string& b);

    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::string Join(const std::vector<std::string>& elements, const std::string& delimiter);

    static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);

    // URL helpers
    static std::string NormalizeUrl(const std::string& input, const std::string& search_engine_url = "https://www.google.com/search?q=");
    static std::string ExtractDomain(const std::string& url);
    static bool IsValidUrl(const std::string& url);

    // Format bytes to human readable (e.g. 15.4 MB, 1.2 GB)
    static std::string FormatBytes(uint64_t bytes);
    static std::string FormatPercentage(double percent);
};

} // namespace nexora::util
