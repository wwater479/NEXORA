#pragma once

#include "tab_item.h"
#include <string>
#include <vector>
#include <functional>

namespace nexora::ui {

enum class SecurityState {
    Secure,         // HTTPS
    Insecure,       // HTTP
    Internal,       // nexora://
    File            // file://
};

class AddressBar {
public:
    using NavigateCallback = std::function<void(const std::string& target_url)>;
    using BookmarkToggleCallback = std::function<void()>;
    using ProtectionToggleCallback = std::function<void()>;

    AddressBar();
    ~AddressBar() = default;

    void SetUrl(const std::string& url);
    std::string GetUrl() const { return current_url_; }

    void SetSecurityState(SecurityState state) { security_state_ = state; }
    SecurityState GetSecurityState() const { return security_state_; }

    void SetBookmarked(bool bookmarked) { is_bookmarked_ = bookmarked; }
    bool IsBookmarked() const { return is_bookmarked_; }

    void SetProtected(bool is_prot) { is_protected_ = is_prot; }
    bool IsProtected() const { return is_protected_; }

    void SetBounds(const Rect& bounds) { bounds_ = bounds; }
    Rect GetBounds() const { return bounds_; }

    Rect GetSecurityIconBounds() const;
    Rect GetBookmarkStarBounds() const;
    Rect GetProtectionShieldBounds() const;

    void OnSubmit(const std::string& input_text);

    void SetCallbacks(NavigateCallback nav_cb, BookmarkToggleCallback bm_cb, ProtectionToggleCallback prot_cb) {
        nav_cb_ = nav_cb;
        bm_cb_ = bm_cb;
        prot_cb_ = prot_cb;
    }

private:
    std::string current_url_;
    SecurityState security_state_ = SecurityState::Internal;
    bool is_bookmarked_ = false;
    bool is_protected_ = false;
    Rect bounds_;

    NavigateCallback nav_cb_;
    BookmarkToggleCallback bm_cb_;
    ProtectionToggleCallback prot_cb_;
};

} // namespace nexora::ui
