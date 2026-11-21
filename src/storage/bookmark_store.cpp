#include "bookmark_store.h"
#include "../util/json_util.h"
#include "../util/string_util.h"
#include "../util/path_util.h"
#include "../util/logger.h"
#include <fstream>
#include <chrono>
#include <algorithm>

namespace nexora::storage {

using namespace nexora::util;

static JsonValue NodeToJson(const std::shared_ptr<BookmarkNode>& node) {
    if (!node) return JsonValue();
    JsonValue obj(JsonValue::ObjectType{});
    obj.Set("id", node->id);
    obj.Set("parent_id", node->parent_id);
    obj.Set("type", node->type == BookmarkNodeType::Folder ? "folder" : "bookmark");
    obj.Set("title", node->title);
    obj.Set("url", node->url);
    obj.Set("favicon_url", node->favicon_url);
    obj.Set("date_added", node->date_added);

    if (node->type == BookmarkNodeType::Folder) {
        JsonValue::ArrayType children_arr;
        for (const auto& child : node->children) {
            children_arr.push_back(NodeToJson(child));
        }
        obj.Set("children", children_arr);
    }
    return obj;
}

static std::shared_ptr<BookmarkNode> JsonToNode(const JsonValue& obj, int64_t& max_id) {
    if (!obj.IsObject()) return nullptr;
    auto node = std::make_shared<BookmarkNode>();
    node->id = obj.Get("id").AsInt64(1);
    node->parent_id = obj.Get("parent_id").AsInt64(0);
    std::string type_str = obj.Get("type").AsString("bookmark");
    node->type = (type_str == "folder") ? BookmarkNodeType::Folder : BookmarkNodeType::Bookmark;
    node->title = obj.Get("title").AsString();
    node->url = obj.Get("url").AsString();
    node->favicon_url = obj.Get("favicon_url").AsString();
    node->date_added = obj.Get("date_added").AsInt64(0);

    if (node->id > max_id) {
        max_id = node->id;
    }

    if (node->type == BookmarkNodeType::Folder) {
        JsonValue children_arr = obj.Get("children");
        if (children_arr.IsArray()) {
            for (const auto& child_val : children_arr.AsArray()) {
                auto child_node = JsonToNode(child_val, max_id);
                if (child_node) {
                    child_node->parent_id = node->id;
                    node->children.push_back(child_node);
                }
            }
        }
    }
    return node;
}

BookmarkStore::BookmarkStore(const std::string& storage_path)
    : storage_path_(storage_path) {
    InitDefaults();
}

BookmarkStore::~BookmarkStore() {
    Save();
}

void BookmarkStore::InitDefaults() {
    root_ = std::make_shared<BookmarkNode>();
    root_->id = 0;
    root_->type = BookmarkNodeType::Folder;
    root_->title = "Root";

    bookmark_bar_ = std::make_shared<BookmarkNode>();
    bookmark_bar_->id = 1;
    bookmark_bar_->parent_id = 0;
    bookmark_bar_->type = BookmarkNodeType::Folder;
    bookmark_bar_->title = "Bookmarks Bar";

    other_bookmarks_ = std::make_shared<BookmarkNode>();
    other_bookmarks_->id = 2;
    other_bookmarks_->parent_id = 0;
    other_bookmarks_->type = BookmarkNodeType::Folder;
    other_bookmarks_->title = "Other Bookmarks";

    root_->children.push_back(bookmark_bar_);
    root_->children.push_back(other_bookmarks_);
    next_id_ = 3;
}

bool BookmarkStore::Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!PathUtil::FileExists(storage_path_)) {
        return true;
    }

    std::ifstream file(storage_path_);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JsonValue root_val = JsonValue::Parse(content);
    if (!root_val.IsObject()) return false;

    int64_t max_id = 2;
    JsonValue roots_obj = root_val.Get("roots");
    if (roots_obj.IsObject()) {
        JsonValue bar_obj = roots_obj.Get("bookmark_bar");
        if (bar_obj.IsObject()) {
            auto loaded_bar = JsonToNode(bar_obj, max_id);
            if (loaded_bar) bookmark_bar_ = loaded_bar;
        }

        JsonValue other_obj = roots_obj.Get("other_bookmarks");
        if (other_obj.IsObject()) {
            auto loaded_other = JsonToNode(other_obj, max_id);
            if (loaded_other) other_bookmarks_ = loaded_other;
        }

        root_->children.clear();
        root_->children.push_back(bookmark_bar_);
        root_->children.push_back(other_bookmarks_);
        next_id_ = max_id + 1;
    }

    LOG_INFO << "Loaded bookmarks from " << storage_path_;
    return true;
}

bool BookmarkStore::Save() {
    std::lock_guard<std::mutex> lock(mutex_);
    JsonValue root_val(JsonValue::ObjectType{});
    JsonValue roots_obj(JsonValue::ObjectType{});

    roots_obj.Set("bookmark_bar", NodeToJson(bookmark_bar_));
    roots_obj.Set("other_bookmarks", NodeToJson(other_bookmarks_));
    root_val.Set("roots", roots_obj);

    PathUtil::EnsureDirectoryExists(PathUtil::GetParentDir(storage_path_));
    std::ofstream file(storage_path_);
    if (!file.is_open()) return false;

    file << root_val.ToString(true);
    return true;
}

std::shared_ptr<BookmarkNode> BookmarkStore::AddBookmark(int64_t parent_id, const std::string& title, const std::string& url, const std::string& favicon) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto parent = FindById(parent_id);
    if (!parent || parent->type != BookmarkNodeType::Folder) {
        parent = bookmark_bar_;
    }

    auto node = std::make_shared<BookmarkNode>();
    node->id = next_id_++;
    node->parent_id = parent->id;
    node->type = BookmarkNodeType::Bookmark;
    node->title = title.empty() ? url : title;
    node->url = url;
    node->favicon_url = favicon;
    node->date_added = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    parent->children.push_back(node);
    return node;
}

std::shared_ptr<BookmarkNode> BookmarkStore::AddFolder(int64_t parent_id, const std::string& title) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto parent = FindById(parent_id);
    if (!parent || parent->type != BookmarkNodeType::Folder) {
        parent = bookmark_bar_;
    }

    auto node = std::make_shared<BookmarkNode>();
    node->id = next_id_++;
    node->parent_id = parent->id;
    node->type = BookmarkNodeType::Folder;
    node->title = title;
    node->date_added = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    parent->children.push_back(node);
    return node;
}

bool BookmarkStore::RemoveNode(int64_t id) {
    if (id <= 2) return false; // Prevent removing root, bar, or other
    std::lock_guard<std::mutex> lock(mutex_);
    return RemoveRecursive(root_, id);
}

bool BookmarkStore::RemoveRecursive(const std::shared_ptr<BookmarkNode>& parent, int64_t id) {
    if (!parent) return false;
    for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
        if ((*it)->id == id) {
            parent->children.erase(it);
            return true;
        }
        if ((*it)->type == BookmarkNodeType::Folder) {
            if (RemoveRecursive(*it, id)) return true;
        }
    }
    return false;
}

bool BookmarkStore::UpdateNode(int64_t id, const std::string& new_title, const std::string& new_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto node = FindById(id);
    if (!node) return false;
    if (!new_title.empty()) node->title = new_title;
    if (node->type == BookmarkNodeType::Bookmark && !new_url.empty()) {
        node->url = new_url;
    }
    return true;
}

bool BookmarkStore::MoveNode(int64_t id, int64_t new_parent_id, size_t index) {
    if (id <= 2) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto target_node = FindById(id);
    auto new_parent = FindById(new_parent_id);
    if (!target_node || !new_parent || new_parent->type != BookmarkNodeType::Folder) {
        return false;
    }

    auto old_parent = FindById(target_node->parent_id);
    if (!old_parent) return false;

    auto it = std::find(old_parent->children.begin(), old_parent->children.end(), target_node);
    if (it != old_parent->children.end()) {
        old_parent->children.erase(it);
    }

    target_node->parent_id = new_parent->id;
    if (index < new_parent->children.size()) {
        new_parent->children.insert(new_parent->children.begin() + index, target_node);
    } else {
        new_parent->children.push_back(target_node);
    }
    return true;
}

std::shared_ptr<BookmarkNode> BookmarkStore::FindById(int64_t id) const {
    if (id == 0) return root_;
    return FindNodeRecursive(root_, id);
}

std::shared_ptr<BookmarkNode> BookmarkStore::FindNodeRecursive(const std::shared_ptr<BookmarkNode>& current, int64_t id) const {
    if (!current) return nullptr;
    if (current->id == id) return current;
    for (const auto& child : current->children) {
        auto found = FindNodeRecursive(child, id);
        if (found) return found;
    }
    return nullptr;
}

std::shared_ptr<BookmarkNode> BookmarkStore::FindByUrl(const std::string& url) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindUrlRecursive(root_, url);
}

std::shared_ptr<BookmarkNode> BookmarkStore::FindUrlRecursive(const std::shared_ptr<BookmarkNode>& current, const std::string& url) const {
    if (!current) return nullptr;
    if (current->type == BookmarkNodeType::Bookmark && current->url == url) {
        return current;
    }
    for (const auto& child : current->children) {
        auto found = FindUrlRecursive(child, url);
        if (found) return found;
    }
    return nullptr;
}

bool BookmarkStore::IsBookmarked(const std::string& url) const {
    return FindByUrl(url) != nullptr;
}

std::vector<std::shared_ptr<BookmarkNode>> BookmarkStore::Search(const std::string& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<BookmarkNode>> results;
    std::string lower_query = StringUtil::ToLower(query);
    SearchRecursive(root_, lower_query, results);
    return results;
}

void BookmarkStore::SearchRecursive(const std::shared_ptr<BookmarkNode>& current, const std::string& lower_query, std::vector<std::shared_ptr<BookmarkNode>>& results) const {
    if (!current) return;
    if (current->id > 0) {
        if (lower_query.empty() ||
            StringUtil::ToLower(current->title).find(lower_query) != std::string::npos ||
            (current->type == BookmarkNodeType::Bookmark && StringUtil::ToLower(current->url).find(lower_query) != std::string::npos)) {
            results.push_back(current);
        }
    }
    for (const auto& child : current->children) {
        SearchRecursive(child, lower_query, results);
    }
}

} // namespace nexora::storage
