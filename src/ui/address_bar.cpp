#include "address_bar.h"
#include "../util/string_util.h"
#include "../core/config_manager.h"

namespace nexora::ui {

using namespace nexora::util;

AddressBar::AddressBar() {}

void AddressBar::SetUrl(const std::string& url) {
    current_url_ = url;
    std::string lower = StringUtil::ToLower(url);
    if (StringUtil::StartsWith(lower, "https://")) {
        security_state_ = SecurityState::Secure;
    } else if (StringUtil::StartsWith(lower, "http://")) {
        security_state_ = SecurityState::Insecure;
    } else if (StringUtil::StartsWith(lower, "file://")) {
        security_state_ = SecurityState::File;
    } else {
        security_state_ = SecurityState::Internal;
    }
}

Rect AddressBar::GetSecurityIconBounds() const {
    int icon_size = 16;
    return Rect{bounds_.x + 8, bounds_.y + (bounds_.height - icon_size) / 2, icon_size, icon_size};
}

Rect AddressBar::GetBookmarkStarBounds() const {
    int btn_size = 20;
    return Rect{bounds_.x + bounds_.width - 28, bounds_.y + (bounds_.height - btn_size) / 2, btn_size, btn_size};
}

Rect AddressBar::GetProtectionShieldBounds() const {
    int btn_size = 20;
    return Rect{bounds_.x + bounds_.width - 56, bounds_.y + (bounds_.height - btn_size) / 2, btn_size, btn_size};
}

void AddressBar::OnSubmit(const std::string& input_text) {
    std::string search_url = core::ConfigManager::Instance().GetConfig().search_engine_url;
    std::string normalized = StringUtil::NormalizeUrl(input_text, search_url);
    SetUrl(normalized);
    if (nav_cb_) {
        nav_cb_(normalized);
    }
}

} // namespace nexora::ui
