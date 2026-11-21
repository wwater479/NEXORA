#pragma once

#include <string>
#include <functional>
#include <cstdint>

#if defined(NEXORA_ENABLE_CEF)
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_display_handler.h"
#include "include/cef_context_menu_handler.h"
#include "include/cef_download_handler.h"
#include "include/cef_request_handler.h"
#include "include/cef_audio_handler.h"
#endif

namespace nexora::engine {

struct BrowserEventCallbacks {
    std::function<void(int64_t tab_id, const std::string& title)> on_title_change;
    std::function<void(int64_t tab_id, const std::string& url)> on_address_change;
    std::function<void(int64_t tab_id, const std::string& favicon_url)> on_favicon_change;
    std::function<void(int64_t tab_id, bool is_loading, double progress)> on_loading_state_change;
    std::function<void(int64_t tab_id, bool is_playing)> on_audio_state_change;
    std::function<void(int64_t tab_id, bool is_secure)> on_security_state_change;
    std::function<void(int64_t tab_id, int error_code, const std::string& error_text, const std::string& failed_url)> on_load_error;
};

class CefClientHandler
#if defined(NEXORA_ENABLE_CEF)
    : public CefClient,
      public CefLifeSpanHandler,
      public CefLoadHandler,
      public CefDisplayHandler,
      public CefContextMenuHandler,
      public CefDownloadHandler,
      public CefRequestHandler
#endif
{
public:
    explicit CefClientHandler(int64_t tab_id, const BrowserEventCallbacks& callbacks);
    virtual ~CefClientHandler() = default;

    int64_t GetTabId() const { return tab_id_; }

#if defined(NEXORA_ENABLE_CEF)
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

    // CefDisplayHandler
    virtual void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;
    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    virtual void OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString>& icon_urls) override;

    // CefLoadHandler
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

    // CefLifeSpanHandler
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefRequestHandler: Strict security enforcement - do not silently bypass TLS errors
    virtual bool OnCertificateError(
        CefRefPtr<CefBrowser> browser,
        cef_errorcode_t cert_error,
        const CefString& request_url,
        CefRefPtr<CefSSLInfo> ssl_info,
        CefRefPtr<CefCallback> callback) override;

    IMPLEMENT_REFCOUNTING(CefClientHandler);
#endif

private:
    int64_t tab_id_;
    BrowserEventCallbacks callbacks_;
};

} // namespace nexora::engine
