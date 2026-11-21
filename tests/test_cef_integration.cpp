#include "test_framework.h"
#include "../src/engine/browser_instance.h"
#include "../src/engine/cef_app_handler.h"
#include "../src/engine/cef_client_handler.h"
#include "../src/engine/process_manager.h"
#include "../src/engine/scheme_handler.h"
#include "../src/engine/script_executor.h"
#include "../src/memory/tab_lifecycle_manager.h"

using namespace nexora::engine;
using namespace nexora::memory;

TEST_CASE(CEFIntegration, BrowserInstanceCreationAndNavigation) {
    BrowserEventCallbacks callbacks;
    std::string latest_url = "";
    std::string latest_title = "";
    bool loading_state = false;

    callbacks.on_address_change = [&](int64_t tid, const std::string& u) {
        (void)tid; latest_url = u;
    };
    callbacks.on_title_change = [&](int64_t tid, const std::string& t) {
        (void)tid; latest_title = t;
    };
    callbacks.on_loading_state_change = [&](int64_t tid, bool is_loading, double prog) {
        (void)tid; (void)prog; loading_state = is_loading;
    };

    BrowserInstance instance(901, 1, nullptr, callbacks);
    ASSERT_TRUE(instance.Create(nexora::ui::Rect{0, 0, 800, 600}, "https://github.com/wwater479/NEXORA"));

    ASSERT_EQ(instance.GetTabId(), 901);
    ASSERT_EQ(instance.GetCurrentUrl(), "https://github.com/wwater479/NEXORA");
    ASSERT_FALSE(instance.IsDiscarded());
    ASSERT_FALSE(instance.IsFrozen());

    // Navigation and history stack
    instance.LoadUrl("https://www.google.com");
    ASSERT_EQ(instance.GetCurrentUrl(), "https://www.google.com");
    ASSERT_TRUE(instance.CanGoBack());
    ASSERT_FALSE(instance.CanGoForward());

    instance.GoBack();
    ASSERT_EQ(instance.GetCurrentUrl(), "https://github.com/wwater479/NEXORA");
    ASSERT_TRUE(instance.CanGoForward());

    instance.GoForward();
    ASSERT_EQ(instance.GetCurrentUrl(), "https://www.google.com");
}

TEST_CASE(CEFIntegration, BrowserInstanceFreezeDiscardAndRestoreCycle) {
    BrowserEventCallbacks callbacks;
    BrowserInstance instance(902, 1, nullptr, callbacks);
    instance.Create(nexora::ui::Rect{0, 0, 1024, 768}, "https://news.ycombinator.com");

    // Register simulated PID
    instance.SetRendererPid(14200);
    ProcessManager::Instance().RegisterProcess(14200, "renderer", 902);
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(902), 14200);

    // Freeze
    ASSERT_TRUE(instance.Freeze());
    ASSERT_TRUE(instance.IsFrozen());
    ASSERT_FALSE(instance.IsDiscarded());

    // Discard
    DiscardedTabState d_state;
    ASSERT_TRUE(instance.Discard(d_state));
    ASSERT_TRUE(instance.IsDiscarded());
    ASSERT_FALSE(instance.IsFrozen());
    ASSERT_EQ(d_state.url, "https://news.ycombinator.com");
    ASSERT_EQ(instance.GetRendererPid(), 0); // Process unregistered and freed

    // Restore
    ASSERT_TRUE(instance.Restore(d_state));
    ASSERT_FALSE(instance.IsDiscarded());
    ASSERT_EQ(instance.GetCurrentUrl(), "https://news.ycombinator.com");

    instance.Destroy();
}

TEST_CASE(CEFIntegration, ProcessManagerRegistrationAndCrashRecovery) {
    ProcessManager::Instance().RegisterProcess(18100, "renderer", 905);
    ProcessManager::Instance().RegisterProcess(18101, "renderer", 906);
    ProcessManager::Instance().RegisterProcess(18102, "gpu", -1);

    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(905), 18100);
    ASSERT_EQ(ProcessManager::Instance().GetActiveRendererCount(), 2);

    // Simulate renderer crash
    ProcessManager::Instance().HandleRendererCrash(18100, 905);
    ASSERT_EQ(ProcessManager::Instance().GetActiveRendererCount(), 1);

    // Simulate GPU crash and recovery
    ProcessManager::Instance().HandleGpuCrash();

    ProcessManager::Instance().UnregisterProcess(18100);
    ProcessManager::Instance().UnregisterProcess(18101);
    ProcessManager::Instance().UnregisterProcess(18102);
}

TEST_CASE(CEFIntegration, SchemeHandlerInternalPagesValidation) {
    std::string newtab_html = SchemeHandler::GetInternalPageHtml("nexora://newtab");
    ASSERT_TRUE(newtab_html.find("<!DOCTYPE html>") != std::string::npos);
    ASSERT_TRUE(newtab_html.find("NEXORA") != std::string::npos);

    std::string settings_html = SchemeHandler::GetInternalPageHtml("nexora://settings");
    ASSERT_TRUE(settings_html.find("Settings") != std::string::npos);

    std::string tm_html = SchemeHandler::GetInternalPageHtml("nexora://taskmanager");
    ASSERT_TRUE(tm_html.find("Task Manager") != std::string::npos);

    std::string mem_html = SchemeHandler::GetInternalPageHtml("nexora://memory");
    ASSERT_TRUE(mem_html.find("Memory Engine") != std::string::npos);

    std::string ver_html = SchemeHandler::GetInternalPageHtml("nexora://version");
    ASSERT_TRUE(ver_html.find("Browser Version") != std::string::npos);
}

TEST_CASE(CEFIntegration, ScriptExecutorGeneratedSnippets) {
    std::string scroll_script = ScriptExecutor::GetScrollPositionScript();
    ASSERT_TRUE(scroll_script.find("window.scrollX") != std::string::npos);

    std::string form_script = ScriptExecutor::GetFormInputDetectionScript();
    ASSERT_TRUE(form_script.find("document.activeElement") != std::string::npos);

    std::string restore_script = ScriptExecutor::GetRestoreScrollScript(100, 250);
    ASSERT_EQ(restore_script, "window.scrollTo(100, 250);");
}
