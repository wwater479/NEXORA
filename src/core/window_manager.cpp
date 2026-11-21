#include "window_manager.h"
#include "../memory/tab_lifecycle_manager.h"
#include "../util/logger.h"
#include <algorithm>

namespace nexora::core {

WindowManager::WindowManager() {}

std::shared_ptr<ui::BrowserWindow> WindowManager::CreateWindow(int x, int y, int width, int height, bool maximized) {
    std::lock_guard<std::mutex> lock(mutex_);
    int win_id = next_window_id_++;
    auto win = std::make_shared<ui::BrowserWindow>(win_id);
    win->Create(x, y, width, height, maximized);
    windows_.push_back(win);
    active_window_id_ = win_id;
    LOG_INFO << "WindowManager: Window " << win_id << " added (Total windows: " << windows_.size() << ")";
    return win;
}

bool WindowManager::CloseWindow(int window_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(windows_.begin(), windows_.end(), [window_id](const std::shared_ptr<ui::BrowserWindow>& w) {
        return w->GetWindowId() == window_id;
    });

    if (it != windows_.end()) {
        (*it)->Destroy();
        windows_.erase(it);
        if (active_window_id_ == window_id) {
            active_window_id_ = windows_.empty() ? -1 : windows_.back()->GetWindowId();
        }
        LOG_INFO << "WindowManager: Window " << window_id << " closed (Remaining: " << windows_.size() << ")";
        return true;
    }
    return false;
}

std::shared_ptr<ui::BrowserWindow> WindowManager::GetWindow(int window_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& w : windows_) {
        if (w->GetWindowId() == window_id) return w;
    }
    return nullptr;
}

std::shared_ptr<ui::BrowserWindow> WindowManager::GetActiveWindow() {
    return GetWindow(active_window_id_);
}

size_t WindowManager::GetWindowCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return windows_.size();
}

bool WindowManager::TransferTab(int from_window_id, int to_window_id, int64_t tab_id) {
    auto from_win = GetWindow(from_window_id);
    auto to_win = GetWindow(to_window_id);
    if (!from_win || !to_win) return false;

    auto tab = from_win->GetTabBar().GetTabById(tab_id);
    if (!tab) return false;

    std::string url = tab->GetUrl();
    std::string title = tab->GetTitle();
    bool is_pinned = tab->IsPinned();
    bool is_protected = tab->IsProtected();

    from_win->GetTabBar().CloseTab(tab_id);
    int64_t new_tab_id = to_win->GetTabBar().CreateTab(title, url);
    auto new_tab = to_win->GetTabBar().GetTabById(new_tab_id);
    if (new_tab) {
        new_tab->SetPinned(is_pinned);
        new_tab->SetProtected(is_protected);
    }
    memory::TabLifecycleManager::Instance().RegisterTab(new_tab_id, to_window_id, url, title);
    LOG_INFO << "Transferred tab from window " << from_window_id << " to window " << to_window_id;
    return true;
}

std::shared_ptr<ui::BrowserWindow> WindowManager::DetachTabToNewWindow(int from_window_id, int64_t tab_id, int screen_x, int screen_y) {
    auto from_win = GetWindow(from_window_id);
    if (!from_win) return nullptr;

    auto tab = from_win->GetTabBar().GetTabById(tab_id);
    if (!tab) return nullptr;

    std::string url = tab->GetUrl();
    std::string title = tab->GetTitle();

    from_win->GetTabBar().CloseTab(tab_id);

    auto new_win = CreateWindow(screen_x, screen_y, 1000, 700);
    if (new_win && new_win->GetTabBar().GetTabCount() > 0) {
        new_win->GetTabBar().GetActiveTab()->SetUrl(url);
        new_win->GetTabBar().GetActiveTab()->SetTitle(title);
        new_win->GetAddressBar().SetUrl(url);
    }

    LOG_INFO << "Detached tab " << tab_id << " to new window at (" << screen_x << ", " << screen_y << ")";
    return new_win;
}

storage::BrowserSessionData WindowManager::CaptureCurrentSession() const {
    std::lock_guard<std::mutex> lock(mutex_);
    storage::BrowserSessionData session;
    session.crashed = false;

    for (const auto& win : windows_) {
        storage::WindowSessionData w_data;
        w_data.window_id = win->GetWindowId();
        w_data.active_tab_index = win->GetTabBar().GetActiveTabIndex();

        for (const auto& tab : win->GetTabBar().GetTabs()) {
            storage::TabSessionData t_data;
            t_data.id = tab->GetId();
            t_data.url = tab->GetUrl();
            t_data.title = tab->GetTitle();
            t_data.favicon_url = tab->GetFaviconUrl();
            t_data.is_pinned = tab->IsPinned();
            t_data.is_muted = tab->IsMuted();
            t_data.is_protected = tab->IsProtected();
            t_data.lifecycle_state = memory::TabLifecycleStateToString(tab->GetLifecycleState());
            t_data.group_id = tab->GetGroupId();
            w_data.tabs.push_back(t_data);
        }
        session.windows.push_back(w_data);
    }
    return session;
}

bool WindowManager::RestoreSession(const storage::BrowserSessionData& session) {
    if (session.windows.empty()) return false;

    CloseAllWindows();

    for (const auto& w_data : session.windows) {
        auto win = CreateWindow(w_data.x, w_data.y, w_data.width, w_data.height, w_data.is_maximized);
        if (!win) continue;

        // Clear initial blank tab
        if (win->GetTabBar().GetTabCount() > 0) {
            win->GetTabBar().CloseTab(win->GetTabBar().GetTabs()[0]->GetId());
        }

        for (const auto& t_data : w_data.tabs) {
            int64_t tab_id = win->GetTabBar().CreateTab(t_data.title, t_data.url);
            auto tab = win->GetTabBar().GetTabById(tab_id);
            if (tab) {
                tab->SetPinned(t_data.is_pinned);
                tab->SetMuted(t_data.is_muted);
                tab->SetProtected(t_data.is_protected);
                tab->SetFaviconUrl(t_data.favicon_url);
            }
            memory::TabLifecycleManager::Instance().RegisterTab(tab_id, win->GetWindowId(), t_data.url, t_data.title);
        }

        if (w_data.active_tab_index >= 0) {
            win->GetTabBar().SelectTabByIndex(static_cast<size_t>(w_data.active_tab_index));
        }
    }

    LOG_INFO << "Restored session with " << session.windows.size() << " windows";
    return true;
}

void WindowManager::CloseAllWindows() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& w : windows_) {
        w->Destroy();
    }
    windows_.clear();
    active_window_id_ = -1;
}

} // namespace nexora::core
