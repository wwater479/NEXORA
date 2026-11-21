#include "cef_client_handler.h"
#include "../util/logger.h"
#include "../util/string_util.h"

namespace nexora::engine {

CefClientHandler::CefClientHandler(int64_t tab_id, const BrowserEventCallbacks& callbacks)
    : tab_id_(tab_id), callbacks_(callbacks) {}

#if defined(NEXORA_ENABLE_CEF)
void CefClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) {
    if (frame->IsMain()) {
        std::string u = url.ToString();
        if (callbacks_.on_address_change) callbacks_.on_address_change(tab_id_, u);
        bool is_secure = util::StringUtil::StartsWith(u, "https://") || util::StringUtil::StartsWith(u, "nexora://");
        if (callbacks_.on_security_state_change) callbacks_.on_security_state_change(tab_id_, is_secure);
    }
}

void CefClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    if (callbacks_.on_title_change) {
        callbacks_.on_title_change(tab_id_, title.ToString());
    }
}

void CefClientHandler::OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString>& icon_urls) {
    if (!icon_urls.empty() && callbacks_.on_favicon_change) {
        callbacks_.on_favicon_change(tab_id_, icon_urls[0].ToString());
    }
}

void CefClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
    if (callbacks_.on_loading_state_change) {
        callbacks_.on_loading_state_change(tab_id_, isLoading, isLoading ? 0.5 : 1.0);
    }
}

void CefClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) {
    if (frame->IsMain()) {
        LOG_WARN << "Load error on tab " << tab_id_ << " (" << failedUrl.ToString() << "): " << errorText.ToString();
        if (callbacks_.on_load_error) {
            callbacks_.on_load_error(tab_id_, static_cast<int>(errorCode), errorText.ToString(), failedUrl.ToString());
        }
    }
}

void CefClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    LOG_INFO << "Browser created for tab " << tab_id_;
}

void CefClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    LOG_INFO << "Browser closing for tab " << tab_id_;
}
#endif

} // namespace nexora::engine
