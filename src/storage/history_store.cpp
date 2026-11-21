#include "history_store.h"
#include "../util/json_util.h"
#include "../util/string_util.h"
#include "../util/path_util.h"
#include "../util/logger.h"
#include <fstream>
#include <chrono>
#include <algorithm>

namespace nexora::storage {

using namespace nexora::util;

HistoryStore::HistoryStore(const std::string& storage_path)
    : storage_path_(storage_path) {}

HistoryStore::~HistoryStore() {
    Save();
}

bool HistoryStore::Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();
    next_id_ = 1;

    if (!PathUtil::FileExists(storage_path_)) {
        return true;
    }

    std::ifstream file(storage_path_);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JsonValue root = JsonValue::Parse(content);
    if (!root.IsObject()) return false;

    JsonValue items_arr = root.Get("history");
    if (items_arr.IsArray()) {
        for (const auto& item_val : items_arr.AsArray()) {
            if (!item_val.IsObject()) continue;
            HistoryItem item;
            item.id = item_val.Get("id").AsInt64(next_id_++);
            item.url = item_val.Get("url").AsString();
            item.title = item_val.Get("title").AsString();
            item.visit_count = item_val.Get("visit_count").AsInt(1);
            item.last_visit_timestamp = item_val.Get("last_visit_timestamp").AsInt64(0);
            item.favicon_url = item_val.Get("favicon_url").AsString();

            if (item.id >= next_id_) {
                next_id_ = item.id + 1;
            }
            items_.push_back(item);
        }
    }

    LOG_INFO << "Loaded " << items_.size() << " history entries from " << storage_path_;
    return true;
}

bool HistoryStore::Save() {
    std::lock_guard<std::mutex> lock(mutex_);
    JsonValue root(JsonValue::ObjectType{});
    JsonValue::ArrayType arr;

    for (const auto& item : items_) {
        JsonValue item_obj(JsonValue::ObjectType{});
        item_obj.Set("id", item.id);
        item_obj.Set("url", item.url);
        item_obj.Set("title", item.title);
        item_obj.Set("visit_count", item.visit_count);
        item_obj.Set("last_visit_timestamp", item.last_visit_timestamp);
        item_obj.Set("favicon_url", item.favicon_url);
        arr.push_back(item_obj);
    }
    root.Set("history", arr);

    PathUtil::EnsureDirectoryExists(PathUtil::GetParentDir(storage_path_));
    std::ofstream file(storage_path_);
    if (!file.is_open()) return false;

    file << root.ToString(true);
    return true;
}

void HistoryStore::AddVisit(const std::string& url, const std::string& title, const std::string& favicon_url) {
    if (url.empty() || StringUtil::StartsWith(url, "nexora://") || StringUtil::StartsWith(url, "about:")) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto& item : items_) {
        if (item.url == url) {
            item.visit_count++;
            item.last_visit_timestamp = now_ts;
            if (!title.empty()) item.title = title;
            if (!favicon_url.empty()) item.favicon_url = favicon_url;
            return;
        }
    }

    HistoryItem new_item;
    new_item.id = next_id_++;
    new_item.url = url;
    new_item.title = title.empty() ? url : title;
    new_item.visit_count = 1;
    new_item.last_visit_timestamp = now_ts;
    new_item.favicon_url = favicon_url;
    items_.push_back(new_item);
}

bool HistoryStore::RemoveItem(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(items_.begin(), items_.end(), [id](const HistoryItem& item) {
        return item.id == id;
    });
    if (it != items_.end()) {
        items_.erase(it, items_.end());
        return true;
    }
    return false;
}

bool HistoryStore::RemoveUrl(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(items_.begin(), items_.end(), [&url](const HistoryItem& item) {
        return item.url == url;
    });
    if (it != items_.end()) {
        items_.erase(it, items_.end());
        return true;
    }
    return false;
}

void HistoryStore::ClearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();
}

void HistoryStore::ClearRange(int64_t start_timestamp, int64_t end_timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(items_.begin(), items_.end(), [start_timestamp, end_timestamp](const HistoryItem& item) {
        return item.last_visit_timestamp >= start_timestamp && item.last_visit_timestamp <= end_timestamp;
    });
    items_.erase(it, items_.end());
}

std::vector<HistoryItem> HistoryStore::Search(const std::string& query, size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HistoryItem> results;
    std::string lower_query = StringUtil::ToLower(query);

    for (const auto& item : items_) {
        if (lower_query.empty() ||
            StringUtil::ToLower(item.title).find(lower_query) != std::string::npos ||
            StringUtil::ToLower(item.url).find(lower_query) != std::string::npos) {
            results.push_back(item);
            if (results.size() >= limit) break;
        }
    }

    std::sort(results.begin(), results.end(), [](const HistoryItem& a, const HistoryItem& b) {
        return a.last_visit_timestamp > b.last_visit_timestamp;
    });
    return results;
}

std::vector<HistoryItem> HistoryStore::GetRecent(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HistoryItem> results = items_;
    std::sort(results.begin(), results.end(), [](const HistoryItem& a, const HistoryItem& b) {
        return a.last_visit_timestamp > b.last_visit_timestamp;
    });
    if (results.size() > limit) {
        results.resize(limit);
    }
    return results;
}

size_t HistoryStore::GetTotalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_.size();
}

} // namespace nexora::storage
