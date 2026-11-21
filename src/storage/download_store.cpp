#include "download_store.h"
#include "../util/json_util.h"
#include "../util/path_util.h"
#include "../util/logger.h"
#include <fstream>
#include <chrono>
#include <algorithm>

namespace nexora::storage {

using namespace nexora::util;

static std::string StateToString(DownloadState state) {
    switch (state) {
        case DownloadState::InProgress: return "in_progress";
        case DownloadState::Paused:     return "paused";
        case DownloadState::Completed:  return "completed";
        case DownloadState::Cancelled:  return "cancelled";
        case DownloadState::Failed:     return "failed";
    }
    return "failed";
}

static DownloadState StringToState(const std::string& str) {
    if (str == "in_progress") return DownloadState::InProgress;
    if (str == "paused")      return DownloadState::Paused;
    if (str == "completed")   return DownloadState::Completed;
    if (str == "cancelled")   return DownloadState::Cancelled;
    return DownloadState::Failed;
}

DownloadStore::DownloadStore(const std::string& storage_path)
    : storage_path_(storage_path) {}

DownloadStore::~DownloadStore() {
    Save();
}

bool DownloadStore::Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();
    next_id_ = 1;

    if (!PathUtil::FileExists(storage_path_)) return true;

    std::ifstream file(storage_path_);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JsonValue root = JsonValue::Parse(content);
    if (!root.IsObject()) return false;

    JsonValue items_arr = root.Get("downloads");
    if (items_arr.IsArray()) {
        for (const auto& item_val : items_arr.AsArray()) {
            if (!item_val.IsObject()) continue;
            DownloadItem item;
            item.id = item_val.Get("id").AsInt64(next_id_++);
            item.url = item_val.Get("url").AsString();
            item.file_name = item_val.Get("file_name").AsString();
            item.target_path = item_val.Get("target_path").AsString();
            item.mime_type = item_val.Get("mime_type").AsString();
            item.total_bytes = item_val.Get("total_bytes").AsInt64(0);
            item.received_bytes = item_val.Get("received_bytes").AsInt64(0);
            item.start_timestamp = item_val.Get("start_timestamp").AsInt64(0);
            item.end_timestamp = item_val.Get("end_timestamp").AsInt64(0);
            item.state = StringToState(item_val.Get("state").AsString("completed"));

            if (item.id >= next_id_) {
                next_id_ = item.id + 1;
            }
            items_.push_back(item);
        }
    }

    LOG_INFO << "Loaded " << items_.size() << " downloads from " << storage_path_;
    return true;
}

bool DownloadStore::Save() {
    std::lock_guard<std::mutex> lock(mutex_);
    JsonValue root(JsonValue::ObjectType{});
    JsonValue::ArrayType arr;

    for (const auto& item : items_) {
        JsonValue item_obj(JsonValue::ObjectType{});
        item_obj.Set("id", item.id);
        item_obj.Set("url", item.url);
        item_obj.Set("file_name", item.file_name);
        item_obj.Set("target_path", item.target_path);
        item_obj.Set("mime_type", item.mime_type);
        item_obj.Set("total_bytes", item.total_bytes);
        item_obj.Set("received_bytes", item.received_bytes);
        item_obj.Set("start_timestamp", item.start_timestamp);
        item_obj.Set("end_timestamp", item.end_timestamp);
        item_obj.Set("state", StateToString(item.state));
        arr.push_back(item_obj);
    }
    root.Set("downloads", arr);

    PathUtil::EnsureDirectoryExists(PathUtil::GetParentDir(storage_path_));
    std::ofstream file(storage_path_);
    if (!file.is_open()) return false;

    file << root.ToString(true);
    return true;
}

int64_t DownloadStore::AddDownload(const std::string& url, const std::string& file_name, const std::string& target_path, int64_t total_bytes, const std::string& mime_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    DownloadItem item;
    item.id = next_id_++;
    item.url = url;
    item.file_name = file_name;
    item.target_path = target_path;
    item.total_bytes = total_bytes;
    item.mime_type = mime_type;
    item.received_bytes = 0;
    item.start_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    item.state = DownloadState::InProgress;
    items_.push_back(item);
    return item.id;
}

void DownloadStore::UpdateProgress(int64_t id, int64_t received_bytes, int64_t total_bytes, double speed_bps) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& item : items_) {
        if (item.id == id) {
            item.received_bytes = received_bytes;
            if (total_bytes > 0) item.total_bytes = total_bytes;
            item.current_speed_bps = speed_bps;
            return;
        }
    }
}

void DownloadStore::UpdateState(int64_t id, DownloadState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& item : items_) {
        if (item.id == id) {
            item.state = state;
            if (state == DownloadState::Completed || state == DownloadState::Cancelled || state == DownloadState::Failed) {
                item.end_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                item.current_speed_bps = 0.0;
            }
            return;
        }
    }
}

bool DownloadStore::RemoveDownload(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(items_.begin(), items_.end(), [id](const DownloadItem& item) {
        return item.id == id;
    });
    if (it != items_.end()) {
        items_.erase(it, items_.end());
        return true;
    }
    return false;
}

void DownloadStore::ClearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();
}

std::vector<DownloadItem> DownloadStore::GetAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_;
}

std::vector<DownloadItem> DownloadStore::GetActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DownloadItem> active;
    for (const auto& item : items_) {
        if (item.state == DownloadState::InProgress || item.state == DownloadState::Paused) {
            active.push_back(item);
        }
    }
    return active;
}

DownloadItem DownloadStore::GetById(int64_t id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& item : items_) {
        if (item.id == id) return item;
    }
    return DownloadItem{};
}

} // namespace nexora::storage
