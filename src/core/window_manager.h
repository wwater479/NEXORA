#pragma once

#include "../ui/browser_window.h"
#include "../storage/session_store.h"
#include <vector>
#include <memory>
#include <mutex>

namespace nexora::core {

class WindowManager {
public:
    static WindowManager& Instance() {
        static WindowManager instance;
        return instance;
    }

    std::shared_ptr<ui::BrowserWindow> CreateWindow(int x = 100, int y = 100, int width = 1200, int height = 800, bool maximized = false);
    bool CloseWindow(int window_id);

    std::shared_ptr<ui::BrowserWindow> GetWindow(int window_id);
    std::shared_ptr<ui::BrowserWindow> GetActiveWindow();
    size_t GetWindowCount() const;

    // Tab movement between windows
    bool TransferTab(int from_window_id, int to_window_id, int64_t tab_id);
    std::shared_ptr<ui::BrowserWindow> DetachTabToNewWindow(int from_window_id, int64_t tab_id, int screen_x, int screen_y);

    // Session save/restore
    storage::BrowserSessionData CaptureCurrentSession() const;
    bool RestoreSession(const storage::BrowserSessionData& session);

    void CloseAllWindows();

private:
    WindowManager();
    ~WindowManager() = default;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<ui::BrowserWindow>> windows_;
    int active_window_id_ = 1;
    int next_window_id_ = 1;
};

} // namespace nexora::core
