#include "session_store.h"
#include "../util/json_util.h"
#include "../util/path_util.h"
#include "../util/logger.h"
#include <fstream>
#include <chrono>

namespace nexora::storage {

using namespace nexora::util;

SessionStore::SessionStore(const std::string& session_file_path)
    : session_file_path_(session_file_path),
      backup_file_path_(session_file_path + ".bak") {}

SessionStore::~SessionStore() {}

bool SessionStore::LoadSession(BrowserSessionData& out_session) {
    std::lock_guard<std::mutex> lock(mutex_);
    out_session.windows.clear();
    out_session.crashed = false;

    std::string target_path = session_file_path_;
    if (!PathUtil::FileExists(target_path)) {
        if (PathUtil::FileExists(backup_file_path_)) {
            target_path = backup_file_path_;
        } else {
            return false;
        }
    }

    std::ifstream file(target_path);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JsonValue root = JsonValue::Parse(content);
    if (!root.IsObject()) return false;

    out_session.crashed = root.Get("crashed").AsBool(false);
    out_session.timestamp = root.Get("timestamp").AsInt64(0);

    JsonValue windows_arr = root.Get("windows");
    if (windows_arr.IsArray()) {
        for (const auto& win_val : windows_arr.AsArray()) {
            if (!win_val.IsObject()) continue;
            WindowSessionData win;
            win.window_id = win_val.Get("window_id").AsInt(0);
            win.x = win_val.Get("x").AsInt(100);
            win.y = win_val.Get("y").AsInt(100);
            win.width = win_val.Get("width").AsInt(1200);
            win.height = win_val.Get("height").AsInt(800);
            win.is_maximized = win_val.Get("is_maximized").AsBool(false);
            win.active_tab_index = win_val.Get("active_tab_index").AsInt(0);

            // Load tabs
            JsonValue tabs_arr = win_val.Get("tabs");
            if (tabs_arr.IsArray()) {
                for (const auto& tab_val : tabs_arr.AsArray()) {
                    if (!tab_val.IsObject()) continue;
                    TabSessionData tab;
                    tab.id = tab_val.Get("id").AsInt64(0);
                    tab.url = tab_val.Get("url").AsString("nexora://newtab");
                    tab.title = tab_val.Get("title").AsString("New Tab");
                    tab.favicon_url = tab_val.Get("favicon_url").AsString("");
                    tab.is_pinned = tab_val.Get("is_pinned").AsBool(false);
                    tab.is_muted = tab_val.Get("is_muted").AsBool(false);
                    tab.is_protected = tab_val.Get("is_protected").AsBool(false);
                    tab.lifecycle_state = tab_val.Get("lifecycle_state").AsString("ACTIVE");
                    tab.group_id = tab_val.Get("group_id").AsInt(-1);
                    tab.last_active_timestamp = tab_val.Get("last_active_timestamp").AsInt64(0);
                    tab.scroll_x = tab_val.Get("scroll_x").AsInt(0);
                    tab.scroll_y = tab_val.Get("scroll_y").AsInt(0);
                    tab.current_history_index = tab_val.Get("current_history_index").AsInt(-1);

                    JsonValue hist_arr = tab_val.Get("navigation_history");
                    if (hist_arr.IsArray()) {
                        for (const auto& h : hist_arr.AsArray()) {
                            tab.navigation_history.push_back(h.AsString());
                        }
                    }
                    win.tabs.push_back(tab);
                }
            }

            // Load groups
            JsonValue groups_arr = win_val.Get("groups");
            if (groups_arr.IsArray()) {
                for (const auto& grp_val : groups_arr.AsArray()) {
                    if (!grp_val.IsObject()) continue;
                    TabGroupSessionData grp;
                    grp.group_id = grp_val.Get("group_id").AsInt(0);
                    grp.title = grp_val.Get("title").AsString();
                    grp.color_hex = grp_val.Get("color_hex").AsString("#1a73e8");
                    grp.is_collapsed = grp_val.Get("is_collapsed").AsBool(false);
                    win.groups.push_back(grp);
                }
            }

            out_session.windows.push_back(win);
        }
    }

    LOG_INFO << "Loaded session with " << out_session.windows.size() << " windows, crashed=" << out_session.crashed;
    return true;
}

bool SessionStore::SaveSession(const BrowserSessionData& session) {
    std::lock_guard<std::mutex> lock(mutex_);
    JsonValue root(JsonValue::ObjectType{});
    root.Set("crashed", session.crashed);
    root.Set("timestamp", std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    JsonValue::ArrayType windows_arr;
    for (const auto& win : session.windows) {
        JsonValue win_obj(JsonValue::ObjectType{});
        win_obj.Set("window_id", win.window_id);
        win_obj.Set("x", win.x);
        win_obj.Set("y", win.y);
        win_obj.Set("width", win.width);
        win_obj.Set("height", win.height);
        win_obj.Set("is_maximized", win.is_maximized);
        win_obj.Set("active_tab_index", win.active_tab_index);

        JsonValue::ArrayType tabs_arr;
        for (const auto& tab : win.tabs) {
            JsonValue tab_obj(JsonValue::ObjectType{});
            tab_obj.Set("id", tab.id);
            tab_obj.Set("url", tab.url);
            tab_obj.Set("title", tab.title);
            tab_obj.Set("favicon_url", tab.favicon_url);
            tab_obj.Set("is_pinned", tab.is_pinned);
            tab_obj.Set("is_muted", tab.is_muted);
            tab_obj.Set("is_protected", tab.is_protected);
            tab_obj.Set("lifecycle_state", tab.lifecycle_state);
            tab_obj.Set("group_id", tab.group_id);
            tab_obj.Set("last_active_timestamp", tab.last_active_timestamp);
            tab_obj.Set("scroll_x", tab.scroll_x);
            tab_obj.Set("scroll_y", tab.scroll_y);
            tab_obj.Set("current_history_index", tab.current_history_index);

            JsonValue::ArrayType hist_arr;
            for (const auto& h : tab.navigation_history) {
                hist_arr.push_back(h);
            }
            tab_obj.Set("navigation_history", hist_arr);
            tabs_arr.push_back(tab_obj);
        }
        win_obj.Set("tabs", tabs_arr);

        JsonValue::ArrayType groups_arr;
        for (const auto& grp : win.groups) {
            JsonValue grp_obj(JsonValue::ObjectType{});
            grp_obj.Set("group_id", grp.group_id);
            grp_obj.Set("title", grp.title);
            grp_obj.Set("color_hex", grp.color_hex);
            grp_obj.Set("is_collapsed", grp.is_collapsed);
            groups_arr.push_back(grp_obj);
        }
        win_obj.Set("groups", groups_arr);

        windows_arr.push_back(win_obj);
    }
    root.Set("windows", windows_arr);

    PathUtil::EnsureDirectoryExists(PathUtil::GetParentDir(session_file_path_));

    // Save backup first
    if (PathUtil::FileExists(session_file_path_)) {
        std::ifstream src(session_file_path_, std::ios::binary);
        std::ofstream dst(backup_file_path_, std::ios::binary);
        dst << src.rdbuf();
    }

    std::ofstream file(session_file_path_);
    if (!file.is_open()) return false;
    file << root.ToString(true);
    return true;
}

bool SessionStore::HasCrashSession() const {
    BrowserSessionData data;
    const_cast<SessionStore*>(this)->LoadSession(data);
    return data.crashed && !data.windows.empty();
}

void SessionStore::ClearCrashFlag() {
    BrowserSessionData data;
    if (LoadSession(data)) {
        data.crashed = false;
        SaveSession(data);
    }
}

} // namespace nexora::storage
