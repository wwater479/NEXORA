# NEXORA Browser: System Architecture Specification

## 1. High-Level Architectural Overview

NEXORA is a high-performance, ultra-lightweight Windows desktop web browser developed in modern C++ (C++17/20), built on a multi-process Chromium architecture with native Win32 UI and an autonomous Memory Optimization Engine.

```
+-----------------------------------------------------------------------------------+
|                                  NEXORA BROWSER                                   |
+-----------------------------------------------------------------------------------+
|  ApplicationCore                                                                  |
|  - AppLifecycleManager (Single instance, cmdline parsing, graceful shutdown)      |
|  - ConfigManager (JSON settings, themes, memory thresholds, protected domains)    |
|  - WindowManager (Multi-window tracking, tab tear-off, session sync)              |
|  - ProfileManager (User profile isolation, incognito ephemeral profiles)         |
|  - UpdateManager (Stable/Beta/Dev channels, version & manifest verification)      |
+-----------------------------------------------------------------------------------+
|  BrowserUI (Native Win32, DWM Glass, Dark/Light Themes, Per-Monitor DPI v2)      |
|  - TabBar (Chrome-style tabs, reordering, dragging, groups, audio/discard badges)  |
|  - AddressBar (Omnibox, URL normalization, SSL security lock, discard shield)     |
|  - NavigationControls (Back, Forward, Reload, Home, Extensions, Profile)          |
|  - BookmarkBar (Quick bookmarks, hierarchical folder dropdowns)                  |
|  - ContextMenu (Tab, Page, Link, Selection menus)                                 |
|  - DownloadUI (Bottom download status bar, progress bar, speed indicator)         |
+-----------------------------------------------------------------------------------+
|  MemoryEngine (Core Memory Optimization Subsystem)                                |
|  - Four-State Tab Lifecycle (ACTIVE, BACKGROUND, FROZEN, DISCARDED)               |
|  - TabPriorityManager (Multi-factor priority scoring & immunity heuristics)       |
|  - MemoryPressureController (Normal, Moderate, Critical system memory evaluation)|
|  - SystemMemoryMonitor (GlobalMemoryStatusEx, GetProcessMemoryInfo, working set)  |
|  - BackgroundResourceManager (Timer throttling, network limits, audio registry)   |
+-----------------------------------------------------------------------------------+
|  DeveloperTools                                                                   |
|  - Browser Task Manager (Per-process & per-tab RAM Private Bytes, WS, CPU %, PID)|
|  - MemoryProfiler (Heap, Blink GC, DOM Nodes, Layout/Image caches, JSON export)   |
|  - CpuMonitor & GpuMonitor (Delta CPU time sampling, GPU crash recovery)          |
|  - DevToolsManager (Chromium DevTools window & remote debugging port 9222)        |
+-----------------------------------------------------------------------------------+
|  Storage & Persistence                                                            |
|  - HistoryStore (Thread-safe visit logging, search, range pruning, JSON)          |
|  - BookmarkStore (Hierarchical tree, node add/update/move, search, JSON)         |
|  - SessionStore (Window & tab geometry, URLs, titles, discarded states, crash)   |
|  - DownloadStore (Active & completed transfers, progress, bytes, speed)           |
+-----------------------------------------------------------------------------------+
|  BrowserEngine (Chromium Embedded Framework - CEF)                                |
|  - CefAppHandler (Memory-optimized command-line switches, custom schemes)        |
|  - CefClientHandler (Lifecycle, load, display, audio, context menu, download)    |
|  - ProcessManager (PID tracking, multi-process orchestration, crash recovery)    |
|  - SchemeHandler (nexora://newtab, settings, history, downloads, taskmanager)     |
|  - ScriptExecutor (DOM state extraction, scroll positions, form input detection)  |
+-----------------------------------------------------------------------------------+
```

---

## 2. Multi-Process Architecture

NEXORA preserves Chromium's multi-process security sandbox while tightly controlling process proliferation:

1. **Browser Main Process (`NEXORA.exe`)**:
   - Manages top-level Win32 windows, user input, tab lifecycle, memory pressure monitoring, and persistent storage.
   - Coordinates subprocess spawning and lifecycle state changes.

2. **Renderer Processes (`NEXORA_Subprocess.exe --type=renderer`)**:
   - Executes Blink web rendering, V8 JavaScript engine, DOM parsing, and CSS styling.
   - Enforces strict Site Isolation (`--site-per-process`) ensuring cross-origin boundaries are isolated into separate sandboxes.
   - Fully sandboxed without direct filesystem or raw network access.
   - Inactive renderers are safely released via NEXORA's 4-State Tab Lifecycle (`FROZEN` $\rightarrow$ `DISCARDED`).

3. **GPU Process (`NEXORA_Subprocess.exe --type=gpu-process`)**:
   - Handles Direct3D 11/12 GPU compositing, WebGL, and rasterization.
   - Automatically monitored by `GpuMonitor` with seamless crash recovery.

4. **Network / Utility Process (`NEXORA_Subprocess.exe --type=utility`)**:
   - Manages TLS certificate verification, HTTP/2 & HTTP/3 multiplexing, and disk caching.

---

## 3. Modular Subsystems & Directory Structure

- `src/core/`: Application lifecycle, configuration, profile isolation, multi-window tracking, update management.
- `src/ui/`: Native Win32 UI components (Tabs, Omnibox, Navigation controls, Bookmarks bar, Context menus, Download drawer, DWM titlebar).
- `src/engine/`: CEF application and client handlers, process management, custom scheme handling (`nexora://`).
- `src/memory/`: Tab lifecycle management, system memory pressure detection, tab prioritization heuristics, background resource throttling.
- `src/devtools/`: Real-time Browser Task Manager, CPU/GPU profiling, memory profiler, DevTools integration.
- `src/storage/`: Atomic JSON stores for History, Bookmarks, Sessions, Downloads.
- `src/util/`: Win32/PSAPI helpers, high-performance thread-safe Logger, UTF-8/16 conversions, zero-dependency JSON parser, path helpers.
