#include "json_util.h"
#include <sstream>
#include <cctype>
#include <iomanip>
#include <stdexcept>

namespace nexora::util {

std::string JsonValue::EscapeString(const std::string& str) {
    std::ostringstream ss;
    ss << '"';
    for (char c : str) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b";  break;
            case '\f': ss << "\\f";  break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                } else {
                    ss << c;
                }
                break;
        }
    }
    ss << '"';
    return ss.str();
}

std::string JsonValue::ToString(bool pretty, int indent) const {
    std::string indent_str = pretty ? std::string(indent * 2, ' ') : "";
    std::string next_indent_str = pretty ? std::string((indent + 1) * 2, ' ') : "";
    std::string nl = pretty ? "\n" : "";
    std::string space = pretty ? " " : "";

    switch (type_) {
        case JsonType::Null:    return "null";
        case JsonType::Boolean: return bool_val_ ? "true" : "false";
        case JsonType::Number: {
            std::ostringstream ss;
            if (num_val_ == static_cast<int64_t>(num_val_)) {
                ss << static_cast<int64_t>(num_val_);
            } else {
                ss << std::setprecision(10) << num_val_;
            }
            return ss.str();
        }
        case JsonType::String:  return EscapeString(str_val_);
        case JsonType::Array: {
            if (arr_val_.empty()) return "[]";
            std::ostringstream ss;
            ss << "[" << nl;
            for (size_t i = 0; i < arr_val_.size(); ++i) {
                if (i > 0) ss << "," << nl;
                ss << next_indent_str << arr_val_[i].ToString(pretty, indent + 1);
            }
            ss << nl << indent_str << "]";
            return ss.str();
        }
        case JsonType::Object: {
            if (obj_val_.empty()) return "{}";
            std::ostringstream ss;
            ss << "{" << nl;
            size_t idx = 0;
            for (const auto& [k, v] : obj_val_) {
                if (idx > 0) ss << "," << nl;
                ss << next_indent_str << EscapeString(k) << ":" << space << v.ToString(pretty, indent + 1);
                idx++;
            }
            ss << nl << indent_str << "}";
            return ss.str();
        }
    }
    return "null";
}

class JsonParser {
public:
    explicit JsonParser(const std::string& src) : src_(src), pos_(0) {}

    JsonValue Parse() {
        SkipWhitespace();
        if (pos_ >= src_.length()) return JsonValue();
        JsonValue val = ParseValue();
        SkipWhitespace();
        return val;
    }

private:
    const std::string& src_;
    size_t pos_;

    void SkipWhitespace() {
        while (pos_ < src_.length() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            pos_++;
        }
    }

    char Peek() const {
        if (pos_ < src_.length()) return src_[pos_];
        return '\0';
    }

    char Get() {
        if (pos_ < src_.length()) return src_[pos_++];
        return '\0';
    }

    JsonValue ParseValue() {
        SkipWhitespace();
        char c = Peek();
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '"') return JsonValue(ParseString());
        if (c == 't' || c == 'f') return ParseBool();
        if (c == 'n') return ParseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber();
        return JsonValue();
    }

    JsonValue ParseObject() {
        Get(); // Consume '{'
        JsonValue::ObjectType obj;
        SkipWhitespace();
        if (Peek() == '}') {
            Get();
            return JsonValue(obj);
        }

        while (pos_ < src_.length()) {
            SkipWhitespace();
            if (Peek() != '"') break;
            std::string key = ParseString();
            SkipWhitespace();
            if (Get() != ':') break;
            JsonValue val = ParseValue();
            obj[key] = val;
            SkipWhitespace();
            char next = Peek();
            if (next == ',') {
                Get();
            } else if (next == '}') {
                Get();
                return JsonValue(obj);
            } else {
                break;
            }
        }
        return JsonValue(obj);
    }

    JsonValue ParseArray() {
        Get(); // Consume '['
        JsonValue::ArrayType arr;
        SkipWhitespace();
        if (Peek() == ']') {
            Get();
            return JsonValue(arr);
        }

        while (pos_ < src_.length()) {
            JsonValue val = ParseValue();
            arr.push_back(val);
            SkipWhitespace();
            char next = Peek();
            if (next == ',') {
                Get();
            } else if (next == ']') {
                Get();
                return JsonValue(arr);
            } else {
                break;
            }
        }
        return JsonValue(arr);
    }

    std::string ParseString() {
        Get(); // Consume leading '"'
        std::string result;
        while (pos_ < src_.length()) {
            char c = Get();
            if (c == '"') {
                return result;
            }
            if (c == '\\') {
                if (pos_ >= src_.length()) break;
                char esc = Get();
                switch (esc) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        if (pos_ + 4 <= src_.length()) {
                            std::string hex_str = src_.substr(pos_, 4);
                            pos_ += 4;
                            try {
                                int code = std::stoi(hex_str, nullptr, 16);
                                if (code < 128) {
                                    result += static_cast<char>(code);
                                } else {
                                    result += '?';
                                }
                            } catch (...) {}
                        }
                        break;
                    }
                    default:   result += esc; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    JsonValue ParseNumber() {
        size_t start = pos_;
        if (Peek() == '-') Get();
        while (pos_ < src_.length() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) ||
                                       src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E' ||
                                       src_[pos_] == '+' || src_[pos_] == '-')) {
            pos_++;
        }
        std::string num_str = src_.substr(start, pos_ - start);
        try {
            double val = std::stod(num_str);
            return JsonValue(val);
        } catch (...) {
            return JsonValue(0.0);
        }
    }

    JsonValue ParseBool() {
        if (src_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return JsonValue(true);
        }
        if (src_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return JsonValue(false);
        }
        return JsonValue(false);
    }

    JsonValue ParseNull() {
        if (src_.substr(pos_, 4) == "null") {
            pos_ += 4;
        }
        return JsonValue();
    }
};

JsonValue JsonValue::Parse(const std::string& json_str) {
    JsonParser parser(json_str);
    return parser.Parse();
}

bool JsonValue::TryParse(const std::string& json_str, JsonValue& out_value) {
    try {
        out_value = Parse(json_str);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace nexora::util
