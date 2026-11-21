#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>

namespace nexora::storage {

enum class BookmarkNodeType {
    Folder,
    Bookmark
};

struct BookmarkNode {
    int64_t id = 0;
    int64_t parent_id = 0;
    BookmarkNodeType type = BookmarkNodeType::Bookmark;
    std::string title;
    std::string url;
    std::string favicon_url;
    int64_t date_added = 0;
    std::vector<std::shared_ptr<BookmarkNode>> children;
};

class BookmarkStore {
public:
    explicit BookmarkStore(const std::string& storage_path);
    ~BookmarkStore();

    bool Load();
    bool Save();

    std::shared_ptr<BookmarkNode> GetRoot() const { return root_; }
    std::shared_ptr<BookmarkNode> GetBookmarkBar() const { return bookmark_bar_; }
    std::shared_ptr<BookmarkNode> GetOtherBookmarks() const { return other_bookmarks_; }

    std::shared_ptr<BookmarkNode> AddBookmark(int64_t parent_id, const std::string& title, const std::string& url, const std::string& favicon = "");
    std::shared_ptr<BookmarkNode> AddFolder(int64_t parent_id, const std::string& title);

    bool RemoveNode(int64_t id);
    bool MoveNode(int64_t id, int64_t new_parent_id, size_t index = static_cast<size_t>(-1));
    bool UpdateNode(int64_t id, const std::string& new_title, const std::string& new_url = "");

    std::shared_ptr<BookmarkNode> FindById(int64_t id) const;
    std::shared_ptr<BookmarkNode> FindByUrl(const std::string& url) const;
    std::vector<std::shared_ptr<BookmarkNode>> Search(const std::string& query) const;

    bool IsBookmarked(const std::string& url) const;

private:
    void InitDefaults();
    std::shared_ptr<BookmarkNode> FindNodeRecursive(const std::shared_ptr<BookmarkNode>& current, int64_t id) const;
    std::shared_ptr<BookmarkNode> FindUrlRecursive(const std::shared_ptr<BookmarkNode>& current, const std::string& url) const;
    void SearchRecursive(const std::shared_ptr<BookmarkNode>& current, const std::string& lower_query, std::vector<std::shared_ptr<BookmarkNode>>& results) const;
    bool RemoveRecursive(const std::shared_ptr<BookmarkNode>& parent, int64_t id);

    std::string storage_path_;
    mutable std::mutex mutex_;
    std::shared_ptr<BookmarkNode> root_;
    std::shared_ptr<BookmarkNode> bookmark_bar_;
    std::shared_ptr<BookmarkNode> other_bookmarks_;
    int64_t next_id_ = 1;
};

} // namespace nexora::storage
