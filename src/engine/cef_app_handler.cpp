#include "cef_app_handler.h"
#include "../util/logger.h"
#include "../util/string_util.h"
#include "../core/config_manager.h"

namespace nexora::engine {

CefAppHandler::CefAppHandler() {}

std::vector<std::string> CefAppHandler::GetOptimizationSwitches() const {
    std::vector<std::string> switches = {
        "--site-per-process",
        "--enable-aggressive-domstorage-flushing",
        "--js-flags=--max-old-space-size=256 --optimize-for-size",
        "--disable-speech-api",
        "--disable-background-networking",
        "--disable-component-update",
        "--enable-surface-synchronization",
        "--enable-gpu-rasterization"
    };
    return switches;
}

std::vector<SwitchJustification> CefAppHandler::GetDocumentedSwitches() const {
    return {
        {
            "--site-per-process",
            "enabled",
            "Enforces strict Site Isolation where every distinct web origin runs in a separate, isolated, sandboxed renderer process. Prevents cross-origin memory disclosure (Spectre) and cross-site data theft.",
            true,
            true
        },
        {
            "--enable-aggressive-domstorage-flushing",
            "enabled",
            "Forces Blink DOM Storage (localStorage/sessionStorage) to commit dirty data to disk immediately, avoiding keeping large uncommitted buffers resident in RAM.",
            true,
            true
        },
        {
            "--js-flags=--max-old-space-size=256 --optimize-for-size",
            "max-old-space=256",
            "Tunes V8 garbage collector heap thresholds: caps old space heap to 256MB per renderer and configures V8 compiler for code compactness without modifying JS semantics or sandboxing.",
            true,
            true
        },
        {
            "--disable-speech-api",
            "disabled",
            "Disables unnecessary Web Speech API runtime background services, saving audio pipeline memory overhead for standard web browsing.",
            true,
            true
        },
        {
            "--disable-background-networking",
            "disabled",
            "Disables speculative background networking, pre-rendering, and pre-fetching when the browser is idle to conserve network sockets and background RAM.",
            true,
            true
        },
        {
            "--disable-component-update",
            "disabled",
            "Prevents background Chrome component updates from triggering uncoordinated disk/memory spikes during active browsing sessions.",
            true,
            true
        },
        {
            "--enable-surface-synchronization",
            "enabled",
            "Eliminates duplicate compositor surface buffers during window resizing, preventing transient multi-megabyte VRAM/RAM spikes.",
            true,
            true
        },
        {
            "--enable-gpu-rasterization",
            "enabled",
            "Delegates page rasterization to hardware GPU pipelines inside the isolated GPU process, offloading CPU main-memory raster buffers.",
            true,
            true
        }
    };
}

bool CefAppHandler::ValidateSecurityConfiguration(const std::vector<std::string>& switches, std::vector<std::string>& out_warnings) {
    bool is_secure = true;
    out_warnings.clear();

    for (const auto& sw : switches) {
        std::string lower = util::StringUtil::ToLower(sw);
        if (lower == "--no-sandbox") {
            out_warnings.push_back("CRITICAL SECURITY VIOLATION: --no-sandbox is forbidden in production builds.");
            is_secure = false;
        } else if (lower == "--disable-site-isolation-trials" || lower == "--disable-site-isolation-for-policy") {
            out_warnings.push_back("CRITICAL SECURITY VIOLATION: Site isolation disabling switches violate process isolation boundaries.");
            is_secure = false;
        } else if (lower == "--single-process") {
            out_warnings.push_back("CRITICAL SECURITY VIOLATION: --single-process destroys multi-process sandboxing and browser crash resiliency.");
            is_secure = false;
        } else if (lower == "--in-process-gpu") {
            out_warnings.push_back("SECURITY WARNING: --in-process-gpu violates GPU process isolation boundary.");
            is_secure = false;
        } else if (lower == "--disable-web-security") {
            out_warnings.push_back("CRITICAL SECURITY VIOLATION: --disable-web-security disables Same-Origin Policy.");
            is_secure = false;
        }
    }

    return is_secure;
}

#if defined(NEXORA_ENABLE_CEF)
void CefAppHandler::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {
    if (process_type.empty()) {
        // Main browser process
        for (const auto& sw : GetOptimizationSwitches()) {
            size_t eq_pos = sw.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = sw.substr(2, eq_pos - 2);
                std::string val = sw.substr(eq_pos + 1);
                command_line->AppendSwitchWithValue(key, val);
            } else {
                std::string key = sw.substr(2);
                command_line->AppendSwitch(key);
            }
        }

        auto dev_cfg = core::ConfigManager::Instance().GetDeveloperConfig();
        if (dev_cfg.enable_remote_debugging) {
            command_line->AppendSwitchWithValue("remote-debugging-port", std::to_string(dev_cfg.remote_debugging_port));
        }

        LOG_INFO << "Configured Chromium command line switches: Strict Site Isolation & Secure Sandboxing enforced";
    }
}

void CefAppHandler::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    registrar->AddCustomScheme("nexora", CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_LOCAL | CEF_SCHEME_OPTION_SECURE);
}

void CefAppHandler::OnContextInitialized() {
    LOG_INFO << "CEF context initialized successfully";
}
#endif

} // namespace nexora::engine
