#pragma once

#include <string>
#include <functional>

namespace nexora::engine {

class ScriptExecutor {
public:
    static std::string GetScrollPositionScript();
    static std::string GetFormInputDetectionScript();
    static std::string GetMemoryOptimizationInjectScript();
    static std::string GetRestoreScrollScript(int scroll_x, int scroll_y);
};

} // namespace nexora::engine
