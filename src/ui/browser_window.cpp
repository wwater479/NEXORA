#include "browser_window.h"
#include "../memory/tab_lifecycle_manager.h"
#include "../devtools/task_manager.h"
#include "../core/config_manager.h"
#include "../util/logger.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nexora::ui {

BrowserWindow::BrowserWindow(int window_id)
    : window_id_(window_id) {

    // Setup callbacks
    tab_bar_.SetCallbacks(
        [this](int64_t tab_id) {
            // Hide all other browsers, show selected
            for (auto& [id, browser] : browsers_) {
                if (id == tab_id) {
                    browser->SetVisible(true);
                    browser->SetFocus(true);
                } else {
                    browser->SetVisible(false);
                }
            }

            auto tab = tab_bar_.GetTabById(tab_id);
            if (tab) {
                address_bar_.SetUrl(tab->GetUrl());
                address_bar_.SetProtected(tab->IsProtected());
                nav_controls_.SetIsLoading(tab->IsLoading());
                auto browser = GetBrowserForTab(tab_id);
                if (browser) {
                    nav_controls_.SetCanGoBack(browser->CanGoBack());
                    nav_controls_.SetCanGoForward(browser->CanGoForward());
                }
                memory::TabLifecycleManager::Instance().SetTabActive(tab_id, true);
            }
        },
        [this](int64_t tab_id) {
            auto b = GetBrowserForTab(tab_id);
            if (b && !b->GetCurrentUrl().empty() && b->GetCurrentUrl() != "nexora://newtab") {
                recently_closed_tabs_.push_back({b->GetCurrentUrl(), b->GetCurrentTitle()});
            }
            CloseBrowserForTab(tab_id);
            memory::TabLifecycleManager::Instance().UnregisterTab(tab_id);
        },
        [this]() {
            int64_t new_id = tab_bar_.CreateTab("New Tab", "nexora://newtab");
            CreateBrowserForTab(new_id, "nexora://newtab");
            memory::TabLifecycleManager::Instance().RegisterTab(new_id, window_id_, "nexora://newtab", "New Tab");
        },
        nullptr,
        nullptr
    );

    address_bar_.SetCallbacks(
        [this](const std::string& target_url) {
            NavigateCurrentTab(target_url);
        },
        [this]() {
            auto tab = tab_bar_.GetActiveTab();
            if (tab) {
                address_bar_.SetBookmarked(!address_bar_.IsBookmarked());
            }
        },
        [this]() {
            auto tab = tab_bar_.GetActiveTab();
            if (tab) {
                bool new_prot = !tab->IsProtected();
                tab->SetProtected(new_prot);
                address_bar_.SetProtected(new_prot);
                memory::TabLifecycleManager::Instance().SetTabProtected(tab->GetId(), new_prot);
            }
        }
    );

    nav_controls_.SetCallbacks(
        [this]() { GoBackCurrentTab(); },
        [this]() { GoForwardCurrentTab(); },
        [this]() { ReloadCurrentTab(); },
        [this]() { NavigateCurrentTab("nexora://newtab"); },
        [this]() { NavigateCurrentTab("nexora://settings"); }
    );

    bookmark_bar_.SetCallback([this](const std::string& url) {
        NavigateCurrentTab(url);
    });

    // Wire Lifecycle Manager discard/restore hooks
    memory::TabLifecycleManager::Instance().SetCallbacks(
        nullptr,
        [this](int64_t tab_id, const memory::DiscardedTabState& d_state) {
            auto b = GetBrowserForTab(tab_id);
            if (b) {
                memory::DiscardedTabState out_st = d_state;
                b->Discard(out_st);
            }
        },
        [this](int64_t tab_id, const memory::DiscardedTabState& r_state) {
            auto b = GetBrowserForTab(tab_id);
            if (b) {
                b->Restore(r_state);
            }
        }
    );
}

BrowserWindow::~BrowserWindow() {
    memory::TabLifecycleManager::Instance().SetCallbacks(nullptr, nullptr, nullptr);
    Destroy();
}

std::shared_ptr<engine::BrowserInstance> BrowserWindow::CreateBrowserForTab(int64_t tab_id, const std::string& url) {
    engine::BrowserEventCallbacks callbacks;
    callbacks.on_address_change = [this](int64_t tid, const std::string& u) {
        auto tab = tab_bar_.GetTabById(tid);
        if (tab) tab->SetUrl(u);
        if (tab_bar_.GetActiveTab() && tab_bar_.GetActiveTab()->GetId() == tid) {
            address_bar_.SetUrl(u);
        }
    };
    callbacks.on_title_change = [this](int64_t tid, const std::string& t) {
        auto tab = tab_bar_.GetTabById(tid);
        if (tab) tab->SetTitle(t);
    };
    callbacks.on_favicon_change = [this](int64_t tid, const std::string& fav) {
        auto tab = tab_bar_.GetTabById(tid);
        if (tab) tab->SetFaviconUrl(fav);
    };
    callbacks.on_loading_state_change = [this](int64_t tid, bool is_loading, double progress) {
        (void)progress;
        auto tab = tab_bar_.GetTabById(tid);
        if (tab) tab->SetLoading(is_loading);
        if (tab_bar_.GetActiveTab() && tab_bar_.GetActiveTab()->GetId() == tid) {
            nav_controls_.SetIsLoading(is_loading);
        }
    };
    callbacks.on_audio_state_change = [this](int64_t tid, bool is_playing) {
        auto tab = tab_bar_.GetTabById(tid);
        if (tab) tab->SetAudioPlaying(is_playing);
        memory::TabLifecycleManager::Instance().SetTabAudioPlaying(tid, is_playing);
    };
    callbacks.on_security_state_change = [this](int64_t tid, bool is_secure) {
        (void)is_secure;
        if (tab_bar_.GetActiveTab() && tab_bar_.GetActiveTab()->GetId() == tid) {
            address_bar_.SetSecurityState(is_secure ? SecurityState::Secure : SecurityState::Insecure);
        }
    };

    auto browser = std::make_shared<engine::BrowserInstance>(tab_id, window_id_, hwnd_, callbacks);
    browser->Create(web_contents_bounds_, url);
    browsers_[tab_id] = browser;
    return browser;
}

void BrowserWindow::CloseBrowserForTab(int64_t tab_id) {
    auto it = browsers_.find(tab_id);
    if (it != browsers_.end()) {
        it->second->Destroy();
        browsers_.erase(it);
    }
}

std::shared_ptr<engine::BrowserInstance> BrowserWindow::GetActiveBrowser() {
    auto tab = tab_bar_.GetActiveTab();
    if (tab) return GetBrowserForTab(tab->GetId());
    return nullptr;
}

std::shared_ptr<engine::BrowserInstance> BrowserWindow::GetBrowserForTab(int64_t tab_id) {
    auto it = browsers_.find(tab_id);
    if (it != browsers_.end()) return it->second;
    return nullptr;
}

bool BrowserWindow::Create(int x, int y, int width, int height, bool maximized) {
    (void)x; (void)y; (void)maximized;
    UpdateLayout(width, height);

#if defined(_WIN32)
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.lpfnWndProc = BrowserWindow::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"NEXORA_BrowserWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0, L"NEXORA_BrowserWindow", L"NEXORA",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, width, height,
        nullptr, nullptr, GetModuleHandleW(nullptr), this
    );

    if (hwnd_) {
        util::Win32Util::EnableDarkModeForHwnd(hwnd_, true);
        if (maximized) {
            ShowWindow((HWND)hwnd_, SW_MAXIMIZE);
        } else {
            ShowWindow((HWND)hwnd_, SW_SHOW);
        }
    }
#endif

    // Initial default tab
    int64_t initial_tab_id = tab_bar_.CreateTab("New Tab", "nexora://newtab");
    CreateBrowserForTab(initial_tab_id, "nexora://newtab");
    memory::TabLifecycleManager::Instance().RegisterTab(initial_tab_id, window_id_, "nexora://newtab", "New Tab");

    LOG_INFO << "BrowserWindow " << window_id_ << " created (" << width << "x" << height << ")";
    return true;
}

void BrowserWindow::Destroy() {
    for (const auto& tab : tab_bar_.GetTabs()) {
        memory::TabLifecycleManager::Instance().UnregisterTab(tab->GetId());
    }
    for (auto& [id, b] : browsers_) {
        b->Destroy();
    }
    browsers_.clear();

#if defined(_WIN32)
    if (hwnd_) {
        DestroyWindow((HWND)hwnd_);
        hwnd_ = nullptr;
    }
#endif
}

void BrowserWindow::Show(bool show) {
#if defined(_WIN32)
    if (hwnd_) {
        ShowWindow((HWND)hwnd_, show ? SW_SHOW : SW_HIDE);
    }
#else
    (void)show;
#endif
}

void BrowserWindow::UpdateLayout(int width, int height) {
    int tab_bar_height = 40;
    int toolbar_height = 42;
    int bookmark_bar_height = bookmark_bar_.IsVisible() ? 32 : 0;
    int download_bar_height = download_ui_.IsVisible() ? 48 : 0;

    tab_bar_bounds_ = Rect{0, 0, width, tab_bar_height};
    tab_bar_.UpdateLayout(0, 0, width, tab_bar_height);

    toolbar_bounds_ = Rect{0, tab_bar_height, width, toolbar_height};
    nav_controls_.UpdateLayout(8, tab_bar_height, toolbar_height);

    int nav_width = nav_controls_.GetTotalWidth();
    int address_bar_x = 8 + nav_width + 8;
    int address_bar_width = width - address_bar_x - 16;
    address_bar_.SetBounds(Rect{address_bar_x, tab_bar_height + 4, address_bar_width, toolbar_height - 8});

    int cur_y = tab_bar_height + toolbar_height;
    if (bookmark_bar_.IsVisible()) {
        bookmark_bar_bounds_ = Rect{0, cur_y, width, bookmark_bar_height};
        bookmark_bar_.UpdateLayout(0, cur_y, width, bookmark_bar_height);
        cur_y += bookmark_bar_height;
    } else {
        bookmark_bar_bounds_ = Rect{0, 0, 0, 0};
    }

    int web_height = height - cur_y - download_bar_height;
    web_contents_bounds_ = Rect{0, cur_y, width, web_height};

    for (auto& [id, b] : browsers_) {
        b->SetBounds(web_contents_bounds_);
    }

    if (download_ui_.IsVisible()) {
        download_ui_.UpdateLayout(0, height - download_bar_height, width, download_bar_height);
    }
}

void BrowserWindow::NavigateCurrentTab(const std::string& url) {
    auto tab = tab_bar_.GetActiveTab();
    if (tab) {
        tab->SetUrl(url);
        tab->SetTitle(url);
        address_bar_.SetUrl(url);
        auto b = GetBrowserForTab(tab->GetId());
        if (b) {
            b->LoadUrl(url);
        }
        memory::TabLifecycleManager::Instance().UpdateTabUrlAndTitle(tab->GetId(), url, url);
        LOG_INFO << "Window " << window_id_ << " tab " << tab->GetId() << " navigating to: " << url;
    }
}

void BrowserWindow::ReloadCurrentTab() {
    auto b = GetActiveBrowser();
    if (b) {
        b->Reload();
    }
}

void BrowserWindow::StopCurrentTab() {
    auto b = GetActiveBrowser();
    if (b) {
        b->Stop();
    }
}

void BrowserWindow::GoBackCurrentTab() {
    auto b = GetActiveBrowser();
    if (b) {
        b->GoBack();
    }
}

void BrowserWindow::GoForwardCurrentTab() {
    auto b = GetActiveBrowser();
    if (b) {
        b->GoForward();
    }
}

void BrowserWindow::ToggleFullscreen() {
    is_fullscreen_ = !is_fullscreen_;
    LOG_INFO << "Fullscreen toggled: " << (is_fullscreen_ ? "ON" : "OFF");
}

void BrowserWindow::ReopenLastClosedTab() {
    if (recently_closed_tabs_.empty()) return;
    auto last_tab = recently_closed_tabs_.back();
    recently_closed_tabs_.pop_back();

    int64_t new_id = tab_bar_.CreateTab(last_tab.second, last_tab.first);
    CreateBrowserForTab(new_id, last_tab.first);
    memory::TabLifecycleManager::Instance().RegisterTab(new_id, window_id_, last_tab.first, last_tab.second);
    LOG_INFO << "Reopened last closed tab: " << last_tab.first;
}

void BrowserWindow::HandleKeyDown(int virtual_key, bool ctrl_pressed, bool shift_pressed, bool alt_pressed) {
    if (alt_pressed) {
        if (virtual_key == 0x25 || virtual_key == 0x27) { // Left / Right arrows
            if (virtual_key == 0x25) {
                LOG_INFO << "Shortcut: Alt+Left (Back)";
                GoBackCurrentTab();
            } else {
                LOG_INFO << "Shortcut: Alt+Right (Forward)";
                GoForwardCurrentTab();
            }
            return;
        } else if (virtual_key == 'D' || virtual_key == 'd') {
            LOG_INFO << "Shortcut: Alt+D (Focus Omnibox)";
            return;
        }
    }

    if (ctrl_pressed) {
        switch (virtual_key) {
            case 'T':
            case 't': {
                if (shift_pressed) {
                    LOG_INFO << "Shortcut: Ctrl+Shift+T (Reopen Closed Tab)";
                    ReopenLastClosedTab();
                } else {
                    LOG_INFO << "Shortcut: Ctrl+T (New Tab)";
                    int64_t id = tab_bar_.CreateTab("New Tab", "nexora://newtab");
                    CreateBrowserForTab(id, "nexora://newtab");
                    memory::TabLifecycleManager::Instance().RegisterTab(id, window_id_, "nexora://newtab", "New Tab");
                }
                break;
            }
            case 'W':
            case 'w': {
                LOG_INFO << "Shortcut: Ctrl+W (Close Tab)";
                auto tab = tab_bar_.GetActiveTab();
                if (tab) {
                    tab_bar_.CloseTab(tab->GetId());
                }
                break;
            }
            case 'R':
            case 'r': {
                if (shift_pressed) {
                    LOG_INFO << "Shortcut: Ctrl+Shift+R (Hard Reload)";
                    auto b = GetActiveBrowser();
                    if (b) b->Reload(true);
                } else {
                    LOG_INFO << "Shortcut: Ctrl+R (Reload)";
                    ReloadCurrentTab();
                }
                break;
            }
            case 'L':
            case 'l': {
                LOG_INFO << "Shortcut: Ctrl+L (Focus Omnibox)";
                break;
            }
            case 'H':
            case 'h': {
                LOG_INFO << "Shortcut: Ctrl+H (History)";
                NavigateCurrentTab("nexora://history");
                break;
            }
            case 'J':
            case 'j': {
                LOG_INFO << "Shortcut: Ctrl+J (Downloads)";
                NavigateCurrentTab("nexora://downloads");
                break;
            }
            case 'D':
            case 'd': {
                LOG_INFO << "Shortcut: Ctrl+D (Bookmark Page)";
                address_bar_.SetBookmarked(!address_bar_.IsBookmarked());
                break;
            }
            case 0x09: { // Tab
                LOG_INFO << "Shortcut: Ctrl+Tab (Cycle Tabs)";
                int cur_idx = tab_bar_.GetActiveTabIndex();
                if (cur_idx >= 0 && tab_bar_.GetTabCount() > 0) {
                    int next_idx = shift_pressed ? ((cur_idx - 1 + tab_bar_.GetTabCount()) % tab_bar_.GetTabCount())
                                                 : ((cur_idx + 1) % tab_bar_.GetTabCount());
                    tab_bar_.SelectTabByIndex(next_idx);
                }
                break;
            }
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': {
                size_t target_idx = static_cast<size_t>(virtual_key - '1');
                tab_bar_.SelectTabByIndex(target_idx);
                break;
            }
            case '9': {
                if (tab_bar_.GetTabCount() > 0) {
                    tab_bar_.SelectTabByIndex(tab_bar_.GetTabCount() - 1);
                }
                break;
            }
        }
    } else if (shift_pressed && virtual_key == 0x1B) { // Shift + Escape
        LOG_INFO << "Shortcut: Shift+Escape (Open Task Manager)";
        NavigateCurrentTab("nexora://taskmanager");
    } else {
        switch (virtual_key) {
            case 0x74: // F5
                if (ctrl_pressed) {
                    auto b = GetActiveBrowser();
                    if (b) b->Reload(true);
                } else {
                    ReloadCurrentTab();
                }
                break;
            case 0x7A: // F11
                ToggleFullscreen();
                break;
            case 0x7B: // F12
                LOG_INFO << "Shortcut: F12 (Open DevTools)";
                auto tab = tab_bar_.GetActiveTab();
                if (tab) {
                    devtools::TaskManager::Instance().RunGarbageCollection(tab->GetId());
                }
                break;
        }
    }
}

#if defined(_WIN32)
LRESULT CALLBACK BrowserWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    BrowserWindow* win = nullptr;
    if (msg == WM_CREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lparam;
        win = (BrowserWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)win);
    } else {
        win = (BrowserWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    switch (msg) {
        case WM_SIZE: {
            if (win) {
                int w = LOWORD(lparam);
                int h = HIWORD(lparam);
                win->UpdateLayout(w, h);
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (win) {
                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                win->HandleKeyDown(static_cast<int>(wparam), ctrl, shift, alt);
            }
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
#endif

} // namespace nexora::ui
