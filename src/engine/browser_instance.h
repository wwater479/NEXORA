#pragma once

#include "cef_client_handler.h"
#include "../ui/tab_item.h"
#include "../memory/memory_types.h"
#include "../util/win32_util.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

#if defined(NEXORA_ENABLE_CEF)
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#endif

namespace nexora::engine {

class BrowserInstance {
public:
    BrowserInstance(int64_t tab_id, int window_id, HWND parent_hwnd, const BrowserEventCallbacks& callbacks);
    ~BrowserInstance();

    bool Create(const ui::Rect& bounds, const std::string& initial_url);
    void Destroy();

    void LoadUrl(const std::string& url, bool is_history_nav = false);
    void ExecuteJavaScript(const std::string& script);

    void GoBack();
    void GoForward();
    void Reload(bool ignore_cache = false);
    void Stop();

    void SetBounds(const ui::Rect& bounds);
    void SetVisible(bool visible);
    void SetFocus(bool focus);
    void SetAudioMuted(bool muted);

    // Lifecycle actions
    bool Freeze();
    bool Discard(memory::DiscardedTabState& out_state);
    bool Restore(const memory::DiscardedTabState& state);

    int64_t GetTabId() const { return tab_id_; }
    int GetWindowId() const { return window_id_; }
    HWND GetParentHwnd() const { return parent_hwnd_; }
    HWND GetBrowserHwnd() const { return browser_hwnd_; }

    std::string GetCurrentUrl() const { return current_url_; }
    std::string GetCurrentTitle() const { return current_title_; }
    std::string GetFaviconUrl() const { return favicon_url_; }

    bool IsLoading() const { return is_loading_; }
    bool IsAudioPlaying() const { return is_audio_playing_; }
    bool IsSecure() const { return is_secure_; }
    bool CanGoBack() const { return can_go_back_; }
    bool CanGoForward() const { return can_go_forward_; }
    bool IsDiscarded() const { return is_discarded_; }
    bool IsFrozen() const { return is_frozen_; }

    uint32_t GetRendererPid() const { return renderer_pid_; }
    void SetRendererPid(uint32_t pid) { renderer_pid_ = pid; }

    int GetScrollX() const { return scroll_x_; }
    int GetScrollY() const { return scroll_y_; }

    const std::vector<std::string>& GetNavigationHistory() const { return navigation_history_; }
    int GetCurrentHistoryIndex() const { return current_history_index_; }

#if defined(NEXORA_ENABLE_CEF)
    CefRefPtr<CefBrowser> GetCefBrowser() const { return cef_browser_; }
    void SetCefBrowser(CefRefPtr<CefBrowser> browser);
#endif

private:
    int64_t tab_id_;
    int window_id_;
    HWND parent_hwnd_;
    HWND browser_hwnd_ = nullptr;
    BrowserEventCallbacks callbacks_;

    std::string current_url_;
    std::string current_title_;
    std::string favicon_url_;

    bool is_loading_ = false;
    bool is_audio_playing_ = false;
    bool is_secure_ = false;
    bool can_go_back_ = false;
    bool can_go_forward_ = false;
    bool is_discarded_ = false;
    bool is_frozen_ = false;
    bool is_visible_ = true;

    uint32_t renderer_pid_ = 0;
    int scroll_x_ = 0;
    int scroll_y_ = 0;
    std::vector<std::string> navigation_history_;
    int current_history_index_ = -1;

    ui::Rect bounds_;

#if defined(NEXORA_ENABLE_CEF)
    CefRefPtr<CefBrowser> cef_browser_;
    CefRefPtr<CefClientHandler> client_handler_;
#endif
};

} // namespace nexora::engine
