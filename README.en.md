# StrataTray

> A native, ultra-light (~10 MB RAM) Windows tray companion for the [Strata](https://github.com/Niko1221/Strata) local LLM engine.

🌐 [简体中文](README.md) ｜ **English**

### Overview

StrataTray is an independent desktop shell for Strata, written in modern C++17 / Qt Widgets. It never modifies Strata: start/stop, version checks and upgrades delegate to Strata's own Python scripts, so the shell keeps working as upstream evolves.

---

### User Guide (for everyone)

1. **Download, unzip, run.** Grab `StrataTray-*.zip` from Releases, extract anywhere — no installer, no admin rights, no console window.
2. **Automatic Strata detection.** Put the extracted `trayapp` folder inside your Strata checkout (e.g. `Strata/trayapp/release/StrataTray.exe`); the app walks up to the folder containing `stratactl.py`. Alternatively set `STRATA_PATH` to any Strata installation.
3. **Start the model.** Double-click `StrataTray.exe` → **Start Service** → wait for the green "Running (Ready)" badge.
4. **Chat from your favourite client.** Click **Copy /v1** and paste the OpenAI-compatible endpoint
   `http://127.0.0.1:8080/v1`
   into the client's API Base URL field (API key can be anything, e.g. `local`). Works with:
   - **NextChat / ChatGPT-Next-Web** → Settings → custom endpoint → Base URL `/v1`
   - **Chatbox** → Settings → OpenAI API Compatible → API Host `/v1`
   - **Cursor** → Settings → Models → Override OpenAI Base URL `/v1`
   - **Cherry Studio** → Settings → Model Provider → OpenAI → API URL `/v1`
   - Any tool supporting `/v1/chat/completions` (Hermes Agent, WorkBuddy custom models, …)
5. **Free your VRAM.** Click **Stop Service**. Closing the window with X only hides to tray; use the tray menu's **Quit** to exit.

> Stop the service before updating. `F5` refreshes logs and health; `Esc` hides to tray.

### Architecture (modular, no single-file god class)

```
src/
├── main.cpp                    # lean entry: single instance / init / event loop
├── core/
│   ├── Config.h/.cpp           # std::filesystem path detection, config, run-at-startup
│   ├── ProcessProbe.h/.cpp     # native Win32 process/port/VRAM probing, log tail
│   └── UpdateService.h/.cpp    # version fetch, JSON parsing, 5-step update, logs
├── ui/
│   ├── I18n.h/.cpp             # JSON dictionary i18n (discovery / English fallback)
│   ├── LanguageMenu.h          # reusable language submenu builder
│   ├── Theme.h                 # built-in dark / light QSS and high-DPI icons
│   ├── ThemeManager.h/.cpp     # theme registry, external .qss, QSettings persistence
│   ├── ThemeMenu.h             # reusable theme submenu builder
│   ├── MainWindow.h/.cpp       # dual-card layout, tabs, status refresh, signals
│   └── TrayApp.h/.cpp          # tray, menus, IPC single instance, Core/UI dispatch
resources/
├── i18n/{en,zh_CN,...}.json    # one language per file, flat key -> text
└── themes/*.qss                # drop a file to add a theme automatically
```

All platform-specific code (`dwmapi.h`, `iphlpapi.h`, registry) is guarded by `#ifdef Q_OS_WIN`, with sensible fallbacks elsewhere.

### Features

- **Resident tray + immersive UI**: native dark title bar and rounded corners on Windows 10/11; the title bar turns light instantly when switching to the light theme. Tray icon reflects service state.
- One-click start / stop (stopping frees VRAM) with state-aware button labels and colors.
- One-click copy of the OpenAI-compatible `/v1` endpoint (or Base URL) with "Copied ✓" feedback.
- Live VRAM bar and last-200-lines log viewer (trailing 64 KB only, error/warning highlighting, auto-scroll, clear view, open log file).
- Local vs. upstream version comparison, release highlights, commits behind, 5-step update progress, automatic version re-check.
- **Multilingual UI (JSON dictionaries, open to contributions)**: every UI string is a flat `key -> text` entry. Language packs are discovered at startup from embedded resources and `release/i18n/`; missing keys fall back to English automatically. Both the ⚙ Settings menu and the tray menu expose a **🌐 Language** submenu with an exclusive `QActionGroup` (current language checked) and instant hot-reload. **Adding a language means adding one JSON file — no C++ changes.**
- **Settings menu**: language submenu, theme submenu, run-at-startup toggle and "Open Strata Folder" live in one compact popup in the top-right corner, keeping both cards symmetric.
- **Open theme system**: a polished dark theme and a clean light theme ship built-in. Switching hot-reloads the global QSS, toggles the native Windows title bar between dark and light, re-tints badges/rich text, and persists via `QSettings`. Contributors can add a new theme by dropping a single `.qss` into `resources/themes/` (see [the guide](resources/themes/README.md)) — the submenu picks it up automatically.
- Single instance (second launch raises the existing window) and optional run-at-startup.
- `F5` refresh, `Esc` hide to tray.

### Contributing Translations

Want StrataTray to speak your language? **No compiler, no C++ knowledge required** — three steps:

1. **Fork this repository**: click Fork on GitHub, then clone it locally (or edit directly in the browser).
2. **Copy the template**: duplicate `resources/i18n/en.json` and rename it for your locale, e.g. `ja_JP.json`, `ko_KR.json`, `fr_FR.json`. The filename without `.json` becomes the language id.
3. **Translate & open a PR**: change only the JSON **values**, never the keys; set `_meta.name` to the language's native name (e.g. `日本語`) and translate the remaining entries. Commit and open a PR — that's it, your translation can ship in the official build!

```json
{
  "_meta": { "name": "日本語", "locale": "ja_JP" },
  "app_title": "Strata ローカル LLM",
  "start_service": "サービスを開始",
  "stop_service": "サービスを停止",
  "service_status": "サービスの状態"
}
```

> Partial translations are perfectly fine: any missing key **falls back to `en.json`** automatically, so the UI never shows blanks or raw keys.
> To preview instantly, drop the JSON into `release/i18n/` and restart the app — the language appears in the submenu. CMake also copies `resources/i18n/*.json` there on every build.
> Please don't rename keys in `en.json`; if a genuinely new UI string is needed, mention the corresponding C++ call site in your PR.

### Prerequisites & Build

Windows 10 19041+ / 11, Qt 6.x MSVC 64-bit, CMake 3.16+, Visual Studio 2022 (MSVC), Ninja (optional), and a working Strata checkout.

```
build.bat
```

The script locates VS via `vswhere.exe`, locates Qt via `QTDIR` / `QT_DIR` / `PATH` / common folders, configures into `build/`, builds Release and runs `windeployqt` into `release/`. Output: `release\StrataTray.exe`.

If Qt is not detected: `set QT_DIR=C:\Qt\6.8.0\msvc2022_64` then `build.bat`.

Manual CMake:

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
cmake --build build --config Release
"%QT_DIR%\bin\windeployqt.exe" --release release\StrataTray.exe
```

### Strata root detection

`STRATA_PATH` / `STRATA_DIR` → walk up from the exe directory looking for `stratactl.py` → current working directory → `Strata` folders on available drives. No machine-specific absolute paths are baked into the source.

### License

Released under the [MIT License](LICENSE). PRs for new language packs and improvements are welcome.

---

## Credits & Invite

> This project was built through deep pair-programming collaboration between **WorkBuddy (hy4)** and **Google Antigravity (Gemini 3.8 Flash)**.

> Try WorkBuddy, the AI pair-programming assistant: [Get your WorkBuddy invite](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> Short link: `https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`
