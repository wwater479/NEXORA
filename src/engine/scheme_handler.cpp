#include "scheme_handler.h"
#include "../util/string_util.h"
#include "../core/update_manager.h"
#include "../memory/tab_lifecycle_manager.h"
#include <sstream>

namespace nexora::engine {

using namespace nexora::util;

bool SchemeHandler::IsInternalScheme(const std::string& url) {
    std::string lower = StringUtil::ToLower(url);
    return StringUtil::StartsWith(lower, "nexora://") ||
           StringUtil::StartsWith(lower, "chrome://") ||
           lower == "about:blank" || lower == "about:version";
}

std::string SchemeHandler::GetInternalPageHtml(const std::string& url) {
    std::string lower = StringUtil::ToLower(url);
    if (lower == "nexora://newtab" || lower == "chrome://newtab" || lower == "about:blank") {
        return GenerateNewTabPageHtml();
    }
    if (lower == "nexora://settings" || lower == "chrome://settings") {
        return GenerateSettingsHtml();
    }
    if (lower == "nexora://history" || lower == "chrome://history") {
        return GenerateHistoryHtml();
    }
    if (lower == "nexora://downloads" || lower == "chrome://downloads") {
        return GenerateDownloadsHtml();
    }
    if (lower == "nexora://taskmanager" || lower == "chrome://taskmanager") {
        return GenerateTaskManagerHtml();
    }
    if (lower == "nexora://memory" || lower == "chrome://memory") {
        return GenerateMemoryHtml();
    }
    if (lower == "nexora://version" || lower == "about:version") {
        return GenerateVersionHtml();
    }
    return GenerateNewTabPageHtml();
}

std::string SchemeHandler::GenerateNewTabPageHtml() {
    auto stats = memory::TabLifecycleManager::Instance().GetMemoryStats();
    std::string saved_ram = StringUtil::FormatBytes(stats.total_memory_saved_bytes);

    std::ostringstream ss;
    ss << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>New Tab</title>
<style>
  :root {
    --bg-color: #202124;
    --text-color: #e8eaed;
    --card-bg: #303134;
    --accent: #8ab4f8;
  }
  body {
    background: var(--bg-color);
    color: var(--text-color);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    margin: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
  }
  .logo {
    font-size: 56px;
    font-weight: 700;
    letter-spacing: -1px;
    margin-bottom: 24px;
    color: var(--accent);
  }
  .search-box {
    width: 580px;
    max-width: 90%;
    background: var(--card-bg);
    border-radius: 24px;
    padding: 12px 24px;
    display: flex;
    align-items: center;
    box-shadow: 0 1px 6px rgba(0,0,0,0.28);
    margin-bottom: 32px;
  }
  .search-box input {
    background: transparent;
    border: none;
    outline: none;
    color: var(--text-color);
    font-size: 16px;
    width: 100%;
  }
  .shortcuts {
    display: flex;
    gap: 16px;
    margin-bottom: 40px;
  }
  .shortcut {
    display: flex;
    flex-direction: column;
    align-items: center;
    text-decoration: none;
    color: var(--text-color);
    font-size: 13px;
    padding: 12px;
    border-radius: 8px;
    transition: background 0.2s;
  }
  .shortcut:hover {
    background: var(--card-bg);
  }
  .shortcut-icon {
    width: 48px;
    height: 48px;
    border-radius: 50%;
    background: #3c4043;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 20px;
    margin-bottom: 8px;
  }
  .memory-widget {
    background: #2a2b2e;
    border: 1px solid #3c4043;
    border-radius: 12px;
    padding: 16px 24px;
    display: flex;
    align-items: center;
    gap: 16px;
    font-size: 14px;
  }
  .badge {
    background: #1e8e3e;
    color: white;
    padding: 4px 10px;
    border-radius: 12px;
    font-weight: 600;
  }
</style>
</head>
<body>
  <div class="logo">NEXORA</div>
  <div class="search-box">
    <input type="text" placeholder="Search Google or type a URL" autofocus id="omnibox-input" onkeydown="if(event.key==='Enter') window.location.href='https://www.google.com/search?q=' + encodeURIComponent(this.value);">
  </div>
  <div class="shortcuts">
    <a class="shortcut" href="https://github.com">
      <div class="shortcut-icon">&#128025;</div>
      <div>GitHub</div>
    </a>
    <a class="shortcut" href="https://www.google.com">
      <div class="shortcut-icon">&#128269;</div>
      <div>Google</div>
    </a>
    <a class="shortcut" href="https://www.wikipedia.org">
      <div class="shortcut-icon">&#128214;</div>
      <div>Wikipedia</div>
    </a>
    <a class="shortcut" href="nexora://taskmanager">
      <div class="shortcut-icon">&#9889;</div>
      <div>Task Manager</div>
    </a>
    <a class="shortcut" href="nexora://memory">
      <div class="shortcut-icon">&#129504;</div>
      <div>Memory</div>
    </a>
    <a class="shortcut" href="nexora://settings">
      <div class="shortcut-icon">&#9881;</div>
      <div>Settings</div>
    </a>
  </div>
  <div class="memory-widget">
    <span>Memory Optimizer Active:</span>
    <span class="badge">)HTML";
    ss << saved_ram;
    ss << R"HTML( Saved</span>
    <span>Discarding inactive tabs automatically</span>
  </div>
</body>
</html>)HTML";
    return ss.str();
}

std::string SchemeHandler::GenerateSettingsHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Settings - NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 40px 80px; }
  h1 { font-size: 28px; border-bottom: 1px solid #3c4043; padding-bottom: 16px; }
  h2 { font-size: 20px; color: #8ab4f8; margin-top: 32px; }
  .setting-row { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid #2d2e30; }
  select, input[type="text"], input[type="number"] { background: #303134; color: #fff; border: 1px solid #5f6368; padding: 8px 12px; border-radius: 4px; }
</style>
</head>
<body>
  <h1>Settings</h1>
  <h2>Memory Optimization Engine</h2>
  <div class="setting-row">
    <div><strong>Enable Memory Optimizer</strong><div>Automatically freeze and discard idle background tabs</div></div>
    <input type="checkbox" checked>
  </div>
  <div class="setting-row">
    <div><strong>Background Tab Freeze Timeout</strong><div>Minutes before idle tab is frozen</div></div>
    <input type="number" value="5" min="1" max="60">
  </div>
  <div class="setting-row">
    <div><strong>Background Tab Discard Timeout</strong><div>Minutes before idle tab renderer is released</div></div>
    <input type="number" value="15" min="2" max="120">
  </div>
  <div class="setting-row">
    <div><strong>Aggressive Discard Under Critical RAM Pressure</strong><div>Discard background tabs when system RAM exceeds 90%</div></div>
    <input type="checkbox" checked>
  </div>
  <h2>Appearance</h2>
  <div class="setting-row">
    <div><strong>Theme</strong><div>Dark, Light, or follow Windows system theme</div></div>
    <select><option selected>System (Dark)</option><option>Dark</option><option>Light</option></select>
  </div>
  <div class="setting-row">
    <div><strong>Show Bookmarks Bar</strong></div>
    <input type="checkbox" checked>
  </div>
  <h2>Search Engine</h2>
  <div class="setting-row">
    <div><strong>Default Search Provider</strong></div>
    <select><option selected>Google</option><option>Bing</option><option>DuckDuckGo</option><option>Ecosia</option></select>
  </div>
</body>
</html>)HTML";
}

std::string SchemeHandler::GenerateHistoryHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><title>History - NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 40px 80px; }
  h1 { font-size: 28px; border-bottom: 1px solid #3c4043; padding-bottom: 16px; }
  .history-item { padding: 12px 16px; border-bottom: 1px solid #2d2e30; display: flex; align-items: center; gap: 16px; }
  .time { color: #9aa0a6; font-size: 13px; width: 80px; }
  .title { color: #8ab4f8; text-decoration: none; font-weight: 500; }
  .url { color: #9aa0a6; font-size: 13px; }
</style>
</head>
<body>
  <h1>Browsing History</h1>
  <div id="history-container">
    <div class="history-item"><span class="time">Today</span><a class="title" href="https://github.com/wwater479/NEXORA">NEXORA Browser Repository - GitHub</a><span class="url">https://github.com/wwater479/NEXORA</span></div>
  </div>
</body>
</html>)HTML";
}

std::string SchemeHandler::GenerateDownloadsHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><title>Downloads - NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 40px 80px; }
  h1 { font-size: 28px; border-bottom: 1px solid #3c4043; padding-bottom: 16px; }
  .download-item { padding: 16px; background: #303134; border-radius: 8px; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
  .btn { background: #8ab4f8; color: #202124; border: none; padding: 6px 16px; border-radius: 4px; cursor: pointer; font-weight: 600; }
</style>
</head>
<body>
  <h1>Downloads</h1>
  <div class="download-item">
    <div><strong>No active downloads</strong><div>Your downloaded files will appear here.</div></div>
  </div>
</body>
</html>)HTML";
}

std::string SchemeHandler::GenerateTaskManagerHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><title>Task Manager - NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 30px; }
  table { width: 100%; border-collapse: collapse; margin-top: 20px; }
  th, td { text-align: left; padding: 10px 14px; border-bottom: 1px solid #3c4043; }
  th { background: #2d2e30; color: #8ab4f8; }
  .badge-active { background: #1e8e3e; color: white; padding: 2px 8px; border-radius: 10px; font-size: 12px; }
  .badge-discarded { background: #d93025; color: white; padding: 2px 8px; border-radius: 10px; font-size: 12px; }
  .badge-frozen { background: #1a73e8; color: white; padding: 2px 8px; border-radius: 10px; font-size: 12px; }
  .actions button { background: #3c4043; color: white; border: none; padding: 4px 8px; border-radius: 4px; cursor: pointer; margin-right: 4px; }
  .actions button:hover { background: #5f6368; }
</style>
</head>
<body>
  <h2>NEXORA Task Manager</h2>
  <table>
    <thead>
      <tr>
        <th>Task</th><th>State</th><th>PID</th><th>RAM (Private)</th><th>Working Set</th><th>CPU %</th><th>Protection</th><th>Actions</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Browser Process</td><td><span class="badge-active">ACTIVE</span></td><td>12840</td><td>45.2 MB</td><td>62.1 MB</td><td>0.8%</td><td>Protected</td><td class="actions">-</td>
      </tr>
      <tr>
        <td>GPU Process</td><td><span class="badge-active">ACTIVE</span></td><td>12844</td><td>32.0 MB</td><td>48.0 MB</td><td>0.2%</td><td>Protected</td><td class="actions">-</td>
      </tr>
      <tr>
        <td>Tab: NEXORA Speed Dial</td><td><span class="badge-active">ACTIVE</span></td><td>12850</td><td>28.4 MB</td><td>38.2 MB</td><td>0.4%</td><td>Auto</td><td class="actions"><button>Discard</button><button>DevTools</button></td>
      </tr>
    </tbody>
  </table>
</body>
</html>)HTML";
}

std::string SchemeHandler::GenerateMemoryHtml() {
    auto stats = memory::TabLifecycleManager::Instance().GetMemoryStats();
    std::string saved_ram = StringUtil::FormatBytes(stats.total_memory_saved_bytes);
    std::string working_set = StringUtil::FormatBytes(stats.total_browser_working_set_bytes);
    std::string private_bytes = StringUtil::FormatBytes(stats.total_browser_private_bytes);

    std::ostringstream ss;
    ss << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><title>Memory Engine - NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 40px; }
  .grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 20px; margin-top: 24px; }
  .card { background: #303134; padding: 20px; border-radius: 8px; border-left: 4px solid #8ab4f8; }
  .metric { font-size: 32px; font-weight: 700; color: #8ab4f8; margin-top: 8px; }
</style>
</head>
<body>
  <h1>NEXORA Memory Engine Status</h1>
  <div class="grid">
    <div class="card">
      <div>Total RAM Saved</div>
      <div class="metric">)HTML";
    ss << saved_ram;
    ss << R"HTML(</div>
    </div>
    <div class="card">
      <div>Browser Working Set</div>
      <div class="metric">)HTML";
    ss << working_set;
    ss << R"HTML(</div>
    </div>
    <div class="card">
      <div>Browser Private Bytes</div>
      <div class="metric">)HTML";
    ss << private_bytes;
    ss << R"HTML(</div>
    </div>
    <div class="card">
      <div>System Memory Load</div>
      <div class="metric">)HTML";
    ss << stats.system_memory_load_percent << "%";
    ss << R"HTML(</div>
    </div>
  </div>
</body>
</html>)HTML";
    return ss.str();
}

std::string SchemeHandler::GenerateVersionHtml() {
    auto v = core::UpdateManager::Instance().GetVersionInfo();
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><title>About NEXORA</title>
<style>
  body { background: #202124; color: #e8eaed; font-family: Segoe UI, sans-serif; padding: 40px 80px; }
  h1 { font-size: 28px; color: #8ab4f8; }
  p { font-size: 16px; line-height: 1.6; }
</style>
</head>
<body>
  <h1>NEXORA Lightweight Browser</h1>
  <p><strong>Browser Version:</strong> )HTML" + v.browser_version + R"HTML(</p>
  <p><strong>Chromium Base:</strong> )HTML" + v.chromium_version + R"HTML(</p>
  <p><strong>CEF Version:</strong> )HTML" + v.cef_version + R"HTML(</p>
  <p><strong>OS:</strong> )HTML" + v.os_version + R"HTML( ()HTML" + v.architecture + R"HTML()</p>
  <p><strong>Build Date:</strong> )HTML" + v.build_date + R"HTML(</p>
  <p>Developed with native Win32 UI and advanced Tab Lifecycle Memory Engine.</p>
</body>
</html>)HTML";
}

std::string SchemeHandler::GenerateErrorPageHtml(const std::string& url, int error_code, const std::string& error_text) {
    return "<!DOCTYPE html><html><body style='background:#202124;color:#e8eaed;font-family:sans-serif;padding:40px;'>"
           "<h2>Failed to load " + url + "</h2>"
           "<p>Error: " + std::to_string(error_code) + " (" + error_text + ")</p>"
           "</body></html>";
}

} // namespace nexora::engine
