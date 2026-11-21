#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace nexora::storage {

struct HistoryItem {
    int64_t id = 0;
    std::string url;
    std::string title;
    int visit_count = 1;
    int64_t last_visit_timestamp = 0;
    std::string favicon_url;
};

class HistoryStore {
public:
    explicit HistoryStore(const std::string& storage_path);
    ~HistoryStore();

    bool Load();
    bool Save();

    void AddVisit(const std::string& url, const std::string& title, const std::string& favicon_url = "");
    bool RemoveItem(int64_t id);
    bool RemoveUrl(const std::string& url);
    void ClearAll();
    void ClearRange(int64_t start_timestamp, int64_t end_timestamp);

    std::vector<HistoryItem> Search(const std::string& query, size_t limit = 100) const;
    std::vector<HistoryItem> GetRecent(size_t limit = 50) const;
    size_t GetTotalCount() const;

private:
    std::string storage_path_;
    mutable std::mutex mutex_;
    std::vector<HistoryItem> items_;
    int64_t next_id_ = 1;
};

} // namespace nexora::storage
