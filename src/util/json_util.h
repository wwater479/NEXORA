#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace nexora::util {

enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    using ObjectType = std::map<std::string, JsonValue>;
    using ArrayType = std::vector<JsonValue>;

    JsonValue() : type_(JsonType::Null) {}
    JsonValue(std::nullptr_t) : type_(JsonType::Null) {}
    JsonValue(bool val) : type_(JsonType::Boolean), bool_val_(val) {}
    JsonValue(int val) : type_(JsonType::Number), num_val_(static_cast<double>(val)) {}
    JsonValue(int64_t val) : type_(JsonType::Number), num_val_(static_cast<double>(val)) {}
    JsonValue(uint64_t val) : type_(JsonType::Number), num_val_(static_cast<double>(val)) {}
    JsonValue(double val) : type_(JsonType::Number), num_val_(val) {}
    JsonValue(const char* val) : type_(JsonType::String), str_val_(val ? val : "") {}
    JsonValue(const std::string& val) : type_(JsonType::String), str_val_(val) {}
    JsonValue(const ArrayType& val) : type_(JsonType::Array), arr_val_(val) {}
    JsonValue(const ObjectType& val) : type_(JsonType::Object), obj_val_(val) {}

    JsonType GetType() const { return type_; }
    bool IsNull() const { return type_ == JsonType::Null; }
    bool IsBool() const { return type_ == JsonType::Boolean; }
    bool IsNumber() const { return type_ == JsonType::Number; }
    bool IsString() const { return type_ == JsonType::String; }
    bool IsArray() const { return type_ == JsonType::Array; }
    bool IsObject() const { return type_ == JsonType::Object; }

    bool AsBool(bool default_val = false) const {
        return type_ == JsonType::Boolean ? bool_val_ : default_val;
    }

    double AsDouble(double default_val = 0.0) const {
        return type_ == JsonType::Number ? num_val_ : default_val;
    }

    int AsInt(int default_val = 0) const {
        return type_ == JsonType::Number ? static_cast<int>(num_val_) : default_val;
    }

    int64_t AsInt64(int64_t default_val = 0) const {
        return type_ == JsonType::Number ? static_cast<int64_t>(num_val_) : default_val;
    }

    std::string AsString(const std::string& default_val = "") const {
        return type_ == JsonType::String ? str_val_ : default_val;
    }

    const ArrayType& AsArray() const { return arr_val_; }
    ArrayType& AsArray() { return arr_val_; }

    const ObjectType& AsObject() const { return obj_val_; }
    ObjectType& AsObject() { return obj_val_; }

    bool HasKey(const std::string& key) const {
        if (type_ != JsonType::Object) return false;
        return obj_val_.find(key) != obj_val_.end();
    }

    JsonValue Get(const std::string& key, const JsonValue& default_val = JsonValue()) const {
        if (type_ != JsonType::Object) return default_val;
        auto it = obj_val_.find(key);
        if (it != obj_val_.end()) return it->second;
        return default_val;
    }

    void Set(const std::string& key, const JsonValue& val) {
        if (type_ != JsonType::Object) {
            type_ = JsonType::Object;
            obj_val_.clear();
        }
        obj_val_[key] = val;
    }

    void Push(const JsonValue& val) {
        if (type_ != JsonType::Array) {
            type_ = JsonType::Array;
            arr_val_.clear();
        }
        arr_val_.push_back(val);
    }

    std::string ToString(bool pretty = false, int indent = 0) const;

    static JsonValue Parse(const std::string& json_str);
    static bool TryParse(const std::string& json_str, JsonValue& out_value);

private:
    JsonType type_ = JsonType::Null;
    bool bool_val_ = false;
    double num_val_ = 0.0;
    std::string str_val_;
    ArrayType arr_val_;
    ObjectType obj_val_;

    static std::string EscapeString(const std::string& str);
};

} // namespace nexora::util
