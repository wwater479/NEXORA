#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace nexora::storage {

enum class DownloadState {
    InProgress,
    Paused,
    Completed,
    Cancelled,
    Failed
};

struct DownloadItem {
    int64_t id = 0;
    std::string url;
    std::string file_name;
    std::string target_path;
    std::string mime_type;
    int64_t total_bytes = 0;
    int64_t received_bytes = 0;
    int64_t start_timestamp = 0;
    int64_t end_timestamp = 0;
    DownloadState state = DownloadState::InProgress;
    double current_speed_bps = 0.0;
};

class DownloadStore {
public:
    explicit DownloadStore(const std::string& storage_path);
    ~DownloadStore();

    bool Load();
    bool Save();

    int64_t AddDownload(const std::string& url, const std::string& file_name, const std::string& target_path, int64_t total_bytes, const std::string& mime_type = "");
    void UpdateProgress(int64_t id, int64_t received_bytes, int64_t total_bytes, double speed_bps);
    void UpdateState(int64_t id, DownloadState state);

    bool RemoveDownload(int64_t id);
    void ClearAll();

    std::vector<DownloadItem> GetAll() const;
    std::vector<DownloadItem> GetActive() const;
    DownloadItem GetById(int64_t id) const;

private:
    std::string storage_path_;
    mutable std::mutex mutex_;
    std::vector<DownloadItem> items_;
    int64_t next_id_ = 1;
};

} // namespace nexora::storage
