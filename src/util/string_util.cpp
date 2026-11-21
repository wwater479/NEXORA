#include "string_util.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <regex>

#if defined(_WIN32)
#include <windows.h>
#else
#include <codecvt>
#include <locale>
#endif

namespace nexora::util {

std::wstring StringUtil::Utf8ToUtf16(const std::string& utf8_str) {
    if (utf8_str.empty()) return L"";
#if defined(_WIN32)
    int count = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (count <= 0) return L"";
    std::wstring result(count - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &result[0], count);
    return result;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    try {
        return converter.from_bytes(utf8_str);
    } catch (...) {
        return L"";
    }
#endif
}

std::string StringUtil::Utf16ToUtf8(const std::wstring& utf16_str) {
    if (utf16_str.empty()) return "";
#if defined(_WIN32)
    int count = WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) return "";
    std::string result(count - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, &result[0], count, nullptr, nullptr);
    return result;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    try {
        return converter.to_bytes(utf16_str);
    } catch (...) {
        return "";
    }
#endif
}

std::string StringUtil::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string StringUtil::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string StringUtil::ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

bool StringUtil::StartsWith(const std::string& str, const std::string& prefix, bool case_sensitive) {
    if (str.length() < prefix.length()) return false;
    if (case_sensitive) {
        return str.compare(0, prefix.length(), prefix) == 0;
    } else {
        return ToLower(str.substr(0, prefix.length())) == ToLower(prefix);
    }
}

bool StringUtil::EndsWith(const std::string& str, const std::string& suffix, bool case_sensitive) {
    if (str.length() < suffix.length()) return false;
    size_t offset = str.length() - suffix.length();
    if (case_sensitive) {
        return str.compare(offset, suffix.length(), suffix) == 0;
    } else {
        return ToLower(str.substr(offset)) == ToLower(suffix);
    }
}

bool StringUtil::EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    return ToLower(a) == ToLower(b);
}

std::vector<std::string> StringUtil::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(str);
    while (std::getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string StringUtil::Join(const std::vector<std::string>& elements, const std::string& delimiter) {
    if (elements.empty()) return "";
    std::ostringstream ss;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << elements[i];
    }
    return ss.str();
}

std::string StringUtil::ReplaceAll(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t start_pos = 0;
    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return result;
}

bool StringUtil::IsValidUrl(const std::string& url) {
    if (url.empty()) return false;
    std::string lower = ToLower(url);
    if (StartsWith(lower, "http://") || StartsWith(lower, "https://") ||
        StartsWith(lower, "file://") || StartsWith(lower, "nexora://") ||
        StartsWith(lower, "chrome://") || StartsWith(lower, "about:")) {
        return true;
    }
    // Check if it looks like a domain name with TLD (e.g. example.com, localhost, 127.0.0.1)
    if (url.find(' ') != std::string::npos) return false;
    size_t dot_pos = url.find('.');
    if (dot_pos != std::string::npos && dot_pos > 0 && dot_pos < url.length() - 1) {
        return true;
    }
    if (StartsWith(lower, "localhost") || StartsWith(lower, "127.0.0.1")) {
        return true;
    }
    return false;
}

std::string StringUtil::NormalizeUrl(const std::string& input, const std::string& search_engine_url) {
    std::string trimmed = Trim(input);
    if (trimmed.empty()) {
        return "nexora://newtab";
    }

    std::string lower = ToLower(trimmed);
    if (StartsWith(lower, "http://") || StartsWith(lower, "https://") ||
        StartsWith(lower, "file://") || StartsWith(lower, "nexora://") ||
        StartsWith(lower, "about:")) {
        return trimmed;
    }

    if (lower == "newtab" || lower == "about:blank" || lower == "nexora:newtab") {
        return "nexora://newtab";
    }

    if (lower == "settings" || lower == "nexora:settings") {
        return "nexora://settings";
    }

    if (lower == "taskmanager" || lower == "nexora:taskmanager") {
        return "nexora://taskmanager";
    }

    if (lower == "memory" || lower == "nexora:memory") {
        return "nexora://memory";
    }

    // Check if it's a domain/IP or a search query
    if (trimmed.find(' ') == std::string::npos) {
        size_t dot_pos = trimmed.find('.');
        if (dot_pos != std::string::npos && dot_pos > 0 && dot_pos < trimmed.length() - 1) {
            return "https://" + trimmed;
        }
        if (StartsWith(lower, "localhost") || StartsWith(lower, "127.0.0.1")) {
            return "http://" + trimmed;
        }
    }

    // Format as search query
    std::string encoded_query = "";
    for (char c : trimmed) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded_query += c;
        } else if (c == ' ') {
            encoded_query += '+';
        } else {
            std::ostringstream escaped;
            escaped << '%' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (static_cast<int>(c) & 0xFF);
            encoded_query += escaped.str();
        }
    }

    return search_engine_url + encoded_query;
}

std::string StringUtil::ExtractDomain(const std::string& url) {
    std::string clean = url;
    size_t proto_pos = clean.find("://");
    if (proto_pos != std::string::npos) {
        clean = clean.substr(proto_pos + 3);
    }
    size_t slash_pos = clean.find('/');
    if (slash_pos != std::string::npos) {
        clean = clean.substr(0, slash_pos);
    }
    size_t colon_pos = clean.find(':');
    if (colon_pos != std::string::npos) {
        clean = clean.substr(0, colon_pos);
    }
    return clean;
}

std::string StringUtil::FormatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(unit_idx == 0 ? 0 : 2) << size << " " << units[unit_idx];
    return ss.str();
}

std::string StringUtil::FormatPercentage(double percent) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << percent << "%";
    return ss.str();
}

} // namespace nexora::util
