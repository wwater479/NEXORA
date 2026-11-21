#include "script_executor.h"

namespace nexora::engine {

std::string ScriptExecutor::GetScrollPositionScript() {
    return "(function() { return JSON.stringify({x: Math.round(window.scrollX || window.pageXOffset || 0), y: Math.round(window.scrollY || window.pageYOffset || 0)}); })();";
}

std::string ScriptExecutor::GetFormInputDetectionScript() {
    return "(function() { var el = document.activeElement; if (!el) return false; var tag = el.tagName.toLowerCase(); return (tag === 'input' || tag === 'textarea' || el.isContentEditable); })();";
}

std::string ScriptExecutor::GetMemoryOptimizationInjectScript() {
    return "(function() { if (window.gc) { try { window.gc(); } catch(e){} } })();";
}

std::string ScriptExecutor::GetRestoreScrollScript(int scroll_x, int scroll_y) {
    return "window.scrollTo(" + std::to_string(scroll_x) + ", " + std::to_string(scroll_y) + ");";
}

} // namespace nexora::engine
