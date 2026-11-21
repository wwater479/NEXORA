#include "browser_instance.h"
#include "script_executor.h"
#include "scheme_handler.h"
#include "process_manager.h"
#include "../util/string_util.h"
#include "../util/logger.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace nexora::engine {

using namespace nexora::util;

BrowserInstance::BrowserInstance(int64_t tab_id, int window_id, HWND parent_hwnd, const BrowserEventCallbacks& callbacks)
    : tab_id_(tab_id), window_id_(window_id), parent_hwnd_(parent_hwnd), callbacks_(callbacks) {}

BrowserInstance::~BrowserInstance() {
    Destroy();
}

bool BrowserInstance::Create(const ui::Rect& bounds, const std::string& initial_url) {
    bounds_ = bounds;
    current_url_ = initial_url.empty() ? "nexora://newtab" : initial_url;
    current_title_ = "New Tab";
    is_discarded_ = false;
    is_frozen_ = false;

    if (!current_url_.empty()) {
        navigation_history_.push_back(current_url_);
        current_history_index_ = 0;
    }

#if defined(NEXORA_ENABLE_CEF)
    client_handler_ = new CefClientHandler(tab_id_, callbacks_);

    CefWindowInfo window_info;
#if defined(_WIN32)
    RECT rect;
    rect.left = bounds.x;
    rect.top = bounds.y;
    rect.right = bounds.x + bounds.width;
    rect.bottom = bounds.y + bounds.height;
    window_info.SetAsChild((HWND)parent_hwnd_, rect);
#endif

    CefBrowserSettings browser_settings;
    browser_settings.javascript = STATE_ENABLED;
    browser_settings.webgl = STATE_ENABLED;
    browser_settings.local_storage = STATE_ENABLED;

    CefBrowserHost::CreateBrowser(window_info, client_handler_, current_url_, browser_settings, nullptr, nullptr);
#endif

    LOG_INFO << "BrowserInstance created for tab " << tab_id_ << " (" << current_url_ << ")";
    if (callbacks_.on_address_change) callbacks_.on_address_change(tab_id_, current_url_);
    if (callbacks_.on_title_change) callbacks_.on_title_change(tab_id_, current_title_);
    return true;
}

void BrowserInstance::Destroy() {
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetHost()->CloseBrowser(true);
        cef_browser_ = nullptr;
    }
    client_handler_ = nullptr;
#endif
    if (renderer_pid_ > 0) {
        ProcessManager::Instance().UnregisterTabFromProcess(renderer_pid_, tab_id_);
        renderer_pid_ = 0;
    }
}

void BrowserInstance::LoadUrl(const std::string& url, bool is_history_nav) {
    if (url.empty()) return;

    if (is_discarded_) {
        memory::DiscardedTabState st;
        st.tab_id = tab_id_;
        st.url = url;
        st.title = url;
        Restore(st);
        return;
    }

    current_url_ = url;
    current_title_ = url;
    is_loading_ = true;

    if (!is_history_nav) {
        if (current_history_index_ >= 0 && current_history_index_ < static_cast<int>(navigation_history_.size()) - 1) {
            navigation_history_.erase(navigation_history_.begin() + current_history_index_ + 1, navigation_history_.end());
        }
        if (navigation_history_.empty() || navigation_history_.back() != url) {
            navigation_history_.push_back(url);
            current_history_index_ = static_cast<int>(navigation_history_.size()) - 1;
        }
    }

    can_go_back_ = (current_history_index_ > 0);
    can_go_forward_ = (current_history_index_ < static_cast<int>(navigation_history_.size()) - 1);
    is_secure_ = StringUtil::StartsWith(StringUtil::ToLower(url), "https://") || StringUtil::StartsWith(StringUtil::ToLower(url), "nexora://");

#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetMainFrame()->LoadURL(url);
    }
#endif

    if (callbacks_.on_address_change) callbacks_.on_address_change(tab_id_, current_url_);
    if (callbacks_.on_title_change) callbacks_.on_title_change(tab_id_, current_title_);
    if (callbacks_.on_security_state_change) callbacks_.on_security_state_change(tab_id_, is_secure_);
    if (callbacks_.on_loading_state_change) callbacks_.on_loading_state_change(tab_id_, true, 0.5);

    is_loading_ = false;
    if (callbacks_.on_loading_state_change) callbacks_.on_loading_state_change(tab_id_, false, 1.0);
}

void BrowserInstance::ExecuteJavaScript(const std::string& script) {
    if (is_discarded_ || script.empty()) return;
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetMainFrame()->ExecuteJavaScript(script, current_url_, 0);
    }
#endif
}

void BrowserInstance::GoBack() {
    if (current_history_index_ > 0) {
        current_history_index_--;
        std::string prev_url = navigation_history_[current_history_index_];
        LoadUrl(prev_url, true);
    }
}

void BrowserInstance::GoForward() {
    if (current_history_index_ < static_cast<int>(navigation_history_.size()) - 1) {
        current_history_index_++;
        std::string next_url = navigation_history_[current_history_index_];
        LoadUrl(next_url, true);
    }
}

void BrowserInstance::Reload(bool ignore_cache) {
    if (is_discarded_) {
        LoadUrl(current_url_);
        return;
    }
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        if (ignore_cache) {
            cef_browser_->ReloadIgnoreCache();
        } else {
            cef_browser_->Reload();
        }
    }
#endif
    LoadUrl(current_url_);
}

void BrowserInstance::Stop() {
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->StopLoad();
    }
#endif
    is_loading_ = false;
    if (callbacks_.on_loading_state_change) callbacks_.on_loading_state_change(tab_id_, false, 1.0);
}

void BrowserInstance::SetBounds(const ui::Rect& bounds) {
    bounds_ = bounds;
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        HWND host_hwnd = (HWND)cef_browser_->GetHost()->GetWindowHandle();
        if (host_hwnd) {
            SetWindowPos(host_hwnd, nullptr, bounds.x, bounds.y, bounds.width, bounds.height, SWP_NOZORDER);
        }
    }
#endif
}

void BrowserInstance::SetVisible(bool visible) {
    is_visible_ = visible;
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetHost()->WasHidden(!visible);
        HWND host_hwnd = (HWND)cef_browser_->GetHost()->GetWindowHandle();
        if (host_hwnd) {
            ShowWindow(host_hwnd, visible ? SW_SHOW : SW_HIDE);
        }
    }
#endif
}

void BrowserInstance::SetFocus(bool focus) {
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetHost()->SetFocus(focus);
    }
#endif
}

void BrowserInstance::SetAudioMuted(bool muted) {
#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetHost()->SetAudioMuted(muted);
    }
#endif
}

bool BrowserInstance::Freeze() {
    if (is_discarded_ || is_frozen_) return false;
    is_frozen_ = true;
    SetVisible(false);
    SetAudioMuted(true);
    ExecuteJavaScript(ScriptExecutor::GetMemoryOptimizationInjectScript());
    LOG_INFO << "BrowserInstance for tab " << tab_id_ << " frozen";
    return true;
}

bool BrowserInstance::Discard(memory::DiscardedTabState& out_state) {
    if (is_discarded_) return false;

    out_state.tab_id = tab_id_;
    out_state.window_id = window_id_;
    out_state.url = current_url_;
    out_state.title = current_title_;
    out_state.favicon_url = favicon_url_;
    out_state.scroll_x = scroll_x_;
    out_state.scroll_y = scroll_y_;
    out_state.navigation_history = navigation_history_;
    out_state.current_history_index = current_history_index_;

#if defined(NEXORA_ENABLE_CEF)
    if (cef_browser_) {
        cef_browser_->GetHost()->CloseBrowser(true);
        cef_browser_ = nullptr;
    }
#endif

    if (renderer_pid_ > 0) {
        ProcessManager::Instance().UnregisterTabFromProcess(renderer_pid_, tab_id_);
        renderer_pid_ = 0;
    }

    is_discarded_ = true;
    is_frozen_ = false;
    LOG_INFO << "BrowserInstance for tab " << tab_id_ << " discarded (Renderer freed if not shared)";
    return true;
}

bool BrowserInstance::Restore(const memory::DiscardedTabState& state) {
    if (!is_discarded_) return true;

    current_url_ = state.url.empty() ? "nexora://newtab" : state.url;
    current_title_ = state.title.empty() ? current_url_ : state.title;
    favicon_url_ = state.favicon_url;
    scroll_x_ = state.scroll_x;
    scroll_y_ = state.scroll_y;
    navigation_history_ = state.navigation_history;
    current_history_index_ = state.current_history_index;

    is_discarded_ = false;
    is_frozen_ = false;

    Create(bounds_, current_url_);

    if (scroll_x_ > 0 || scroll_y_ > 0) {
        ExecuteJavaScript(ScriptExecutor::GetRestoreScrollScript(scroll_x_, scroll_y_));
    }

    LOG_INFO << "BrowserInstance for tab " << tab_id_ << " restored -> " << current_url_
             << " (scroll: " << scroll_x_ << ", " << scroll_y_ << ")";
    return true;
}

#if defined(NEXORA_ENABLE_CEF)
void BrowserInstance::SetCefBrowser(CefRefPtr<CefBrowser> browser) {
    cef_browser_ = browser;
    if (cef_browser_) {
        browser_hwnd_ = (HWND)cef_browser_->GetHost()->GetWindowHandle();
    }
}
#endif

} // namespace nexora::engine
