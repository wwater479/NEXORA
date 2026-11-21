# NEXORA Browser: Developer Guide & Monitoring Tools

## 1. Developer Mode Features

NEXORA includes a built-in Developer Mode designed for software architects, performance engineers, and web developers.

### Key Capabilities:
- **Dedicated Browser Task Manager**: Inspect real-time Private Bytes, Working Set, CPU %, GPU Memory, and Network I/O per process and per tab.
- **Tab Protection Locking**: Protect critical tabs (e.g. cloud IDEs, streaming consoles, staging dashboards) from automatic memory discarding with a single click.
- **Manual Discard / Freeze / GC**: Trigger instant tab freezing, discarding, or V8 garbage collection on demand.
- **Chromium DevTools Integration**: Docked or detached DevTools inspector for any web page via `F12` or right-click $\rightarrow$ "Inspect Element".
- **Remote Debugging Protocol**: Connect external IDEs or automated testing tools to `http://localhost:9222`.

---

## 2. Dedicated Browser Task Manager (`Shift + Esc` or `nexora://taskmanager`)

The NEXORA Task Manager provides granular visibility into all browser processes and tab lifecycle states:

```
+-----------------------------------------------------------------------------------------------------------------+
|                                              NEXORA TASK MANAGER                                                |
+------------------------+------------+-------+-----------------+-----------------+-------+-------------+---------+
| Task Name              | State      | PID   | RAM (Private)   | Working Set     | CPU % | Protection  | Actions |
+------------------------+------------+-------+-----------------+-----------------+-------+-------------+---------+
| Browser Process        | ACTIVE     | 12840 | 45.2 MB         | 62.1 MB         | 0.8%  | Protected   | -       |
| GPU Process            | ACTIVE     | 12844 | 32.0 MB         | 48.0 MB         | 0.2%  | Protected   | -       |
| Network Service        | ACTIVE     | 12848 | 18.0 MB         | 24.0 MB         | 0.1%  | Protected   | -       |
| Tab: NEXORA Speed Dial | ACTIVE     | 12850 | 28.4 MB         | 38.2 MB         | 0.4%  | Auto        | [Disc]  |
| Tab: GitHub Repo       | FROZEN     | 12854 | 12.0 MB         | 18.0 MB         | 0.0%  | Auto        | [Disc]  |
| Tab: Cloud Staging     | ACTIVE     | 12858 | 58.0 MB         | 74.0 MB         | 0.9%  | [SHIELD ON] | [Disc]  |
| Tab: Wikipedia         | DISCARDED  | -     | 0.0 MB          | 0.0 MB          | 0.0%  | Auto        | [Rest]  |
+------------------------+------------+-------+-----------------+-----------------+-------+-------------+---------+
| [ End Process ] [ Force Discard ] [ Force Freeze ] [ Toggle Discard Protection ] [ Run Garbage Collection (GC) ]|
+-----------------------------------------------------------------------------------------------------------------+
```

---

## 3. Keyboard Shortcuts Reference

| Shortcut | Action |
|---|---|
| `Ctrl + T` | Open new tab |
| `Ctrl + W` | Close active tab |
| `Ctrl + Tab` / `Ctrl + Shift + Tab` | Switch to next / previous tab |
| `Ctrl + 1` .. `Ctrl + 8` | Switch to Tab index 1 through 8 |
| `Ctrl + 9` | Switch to last tab |
| `Ctrl + Shift + T` | Reopen last closed tab |
| `Ctrl + L` / `Alt + D` | Focus Omnibox / address bar |
| `Ctrl + R` / `F5` | Reload current page |
| `Ctrl + Shift + R` / `Ctrl + F5` | Hard reload (bypass cache) |
| `Ctrl + H` | Open Browsing History (`nexora://history`) |
| `Ctrl + J` | Open Downloads Manager (`nexora://downloads`) |
| `Ctrl + D` | Bookmark active page |
| `F11` | Toggle Fullscreen mode |
| `F12` / `Ctrl + Shift + I` | Open Chromium DevTools |
| `Shift + Esc` | Open NEXORA Task Manager (`nexora://taskmanager`) |

---

## 4. Internal Scheme URLs

NEXORA provides clean, modern internal HTML5/CSS3 pages:

- `nexora://newtab`: Chrome-inspired Speed Dial with live RAM saved counter.
- `nexora://taskmanager`: Dedicated browser task manager.
- `nexora://memory`: Real-time memory engine analytics and Working Set graphs.
- `nexora://settings`: Browser configuration and memory engine threshold controls.
- `nexora://history`: Chronological visit history with instant search and pruning.
- `nexora://downloads`: Active and completed downloads with file reveal.
- `nexora://version`: Detailed browser, Chromium base, CEF, and OS build metadata.
