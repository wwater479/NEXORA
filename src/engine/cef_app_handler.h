#pragma once

#include <string>
#include <vector>

#if defined(NEXORA_ENABLE_CEF)
#include "include/cef_app.h"
#include "include/cef_browser_process_handler.h"
#include "include/cef_render_process_handler.h"
#endif

namespace nexora::engine {

struct SwitchJustification {
    std::string switch_name;
    std::string default_value;
    std::string justification;
    bool preserves_sandbox;
    bool preserves_site_isolation;
};

class CefAppHandler
#if defined(NEXORA_ENABLE_CEF)
    : public CefApp,
      public CefBrowserProcessHandler,
      public CefRenderProcessHandler
#endif
{
public:
    CefAppHandler();
    virtual ~CefAppHandler() = default;

    // Command-line optimization arguments & justifications
    std::vector<std::string> GetOptimizationSwitches() const;
    std::vector<SwitchJustification> GetDocumentedSwitches() const;

    // Security audit validation
    static bool ValidateSecurityConfiguration(const std::vector<std::string>& switches, std::vector<std::string>& out_warnings);

#if defined(NEXORA_ENABLE_CEF)
    // CefApp methods
    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }
    virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override;
    virtual void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

    // CefBrowserProcessHandler methods
    virtual void OnContextInitialized() override;

    IMPLEMENT_REFCOUNTING(CefAppHandler);
#endif
};

} // namespace nexora::engine
