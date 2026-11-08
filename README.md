# StrataTray

> 为 [Strata](https://github.com/Niko1221/Strata) 本地大模型推理引擎打造的原生、超轻量 Windows 托盘管理工具（常驻内存约 10 MB）。
> A native, ultra-light (~10 MB RAM) Windows tray companion for the Strata local LLM engine.

**致谢 / Acknowledgement**

> 本项目由 **WorkBuddy (hy4)** 与 **Google Antigravity (Gemini 3.8 Flash)** 深度协作、结对编程配合完成。
> This project was built through deep pair-programming collaboration between **WorkBuddy (hy4)** and **Google Antigravity (Gemini 3.8 Flash)**.

> 欢迎使用高效的 AI 结对编程助手 WorkBuddy：[点击获取 WorkBuddy 专属邀请体验](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> 短链：`https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`
> Try WorkBuddy, the AI pair-programming assistant: [Get your WorkBuddy invite](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)

[简体中文](#简体中文) · [English](#english)

---

## 简体中文

### 普通用户使用指南（开箱即用）

1. **下载解压即用**：从 Releases 下载 `StrataTray-*.zip`，解压到任意位置；不需要安装、不需要管理员权限、不会弹黑框。
2. **自动识别 Strata**：把解压后的 `trayapp` 文件夹放在 Strata 目录下（例如 `Strata/trayapp/release/StrataTray.exe`），程序会自动向上找到含 `stratactl.py` 的根目录；也可设置环境变量 `STRATA_PATH` 指向任意 Strata 安装位置。
3. **启动本地模型**：双击 `StrataTray.exe` → 点「启动服务」，等状态徽章变成绿色「运行中（就绪）」。
4. **在第三方客户端里开始对话**：点窗口里的 **「复制 /v1」**，把 OpenAI 兼容接口地址
   `http://127.0.0.1:8080/v1`
   粘贴到客户端的「API 地址 / Base URL」栏，API Key 随便填（如 `local`），模型选择本地模型即可。适用于：
   - **NextChat / ChatGPT-Next-Web**：设置 → 自定义接口 → Base URL 填 `/v1`
   - **Chatbox**：设置 → 模型提供方 → OpenAI API Compatible → API 主机填 `/v1`
   - **Cursor**：Settings → Models → OpenAI Base URL 填 `/v1`（Override OpenAI Base URL）
   - **Cherry Studio**：设置 → 模型服务 → OpenAI → API 地址填 `/v1`
   - 以及任何支持 OpenAI `/v1/chat/completions` 的工具（Hermes Agent、WorkBuddy 自定义模型等）
5. **用完释放显存**：点「停止服务」，显存立即归还；关窗口右上角 X 只是缩回托盘，托盘菜单「退出」才真正退出。

> 提示：服务运行中不能升级；先停止服务，再点「立即更新」。`F5` 刷新日志与状态，`Esc` 缩回托盘。

### 简介

StrataTray 是 Strata 官方仓库之外的**独立桌面外壳**，用现代 C++17 / Qt Widgets 编写，不修改 Strata 本体。启停、版本检查、升级全部委托给 Strata 自带的 Python 脚本，本项目只做展示与调度，因此上游频繁更新时无需重新适配。

**架构（模块化，告别单文件上帝类）**：

```
src/
├── main.cpp                    # 精简入口：单实例 / 初始化 / 事件循环
├── core/
│   ├── Config.h/.cpp           # std::filesystem 路径探测、单例配置、开机自启
│   ├── ProcessProbe.h/.cpp     # Win32 原生进程/端口/显存探活、日志尾部读取
│   └── UpdateService.h/.cpp    # 版本拉取、JSON 解析、5 步更新、日志收集
├── ui/
│   ├── I18n.h/.cpp             # JSON 键值字典多语言框架（动态发现 / 英文回退）
│   ├── LanguageMenu.h          # 可复用的「界面语言」二级子菜单构建器
│   ├── Theme.h                 # 内置 dark / light QSS 与高 DPI 图标
│   ├── ThemeManager.h/.cpp     # 主题注册表、外部 .qss 发现、QSettings 持久化
│   ├── ThemeMenu.h             # 可复用的「外观主题」二级子菜单构建器
│   ├── MainWindow.h/.cpp       # 双卡片布局、Tab、状态刷新、按钮信号
│   └── TrayApp.h/.cpp          # 托盘、菜单、IPC 单实例、Core/UI 调度
resources/
├── i18n/{en,zh_CN,...}.json    # 语言包：每个文件一个语种，扁平 key -> 文案
└── themes/*.qss                # 外部主题：放一个文件即自动进入菜单
```

平台相关代码（`dwmapi.h`、`iphlpapi.h`、注册表）严格包在 `#ifdef Q_OS_WIN` 中，其它平台提供合理 Fallback。

### 主要特性

- **常驻托盘 + 沉浸式暗色界面**：Win10/Win11 原生暗色标题栏与圆角，托盘图标随状态变色。
- **一键启动 / 停止**（停止释放显存），启动中 / 就绪 / 异常有明确按钮文案与颜色引导。
- **一键复制 OpenAI 兼容接口** `/v1`（或 Base URL），带「已复制 ✓」绿色反馈。
- **显存与日志实时流**：微型显存条、最后 200 行日志（只读末尾 64 KB，日志再大也不卡）、错误/警告着色、自动滚动、清空视图、打开日志文件。
- **版本对比与一键热更新**：本地 / 线上版本号、官方发布要点、落后提交数、5 步进度与耗时，完成后自动复核版本。
- **多语言（JSON 字典，开放贡献）**：所有界面文案都是 `key -> 文案` 的扁平 JSON 词条；语言包在启动时从内嵌资源与 `release/i18n/` 动态发现，缺失词条自动回退英文。齿轮设置菜单与托盘菜单均提供「🌐 界面语言」二级子菜单，`QActionGroup` 互斥单选、当前语言带勾选，点击即时热重载。**新增语种只需加一个 JSON 文件**，无需改 C++。
- **设置菜单**：界面语言、外观主题、开机自启动开关、打开 Strata 根目录，收在右上角 ⚙ 弹出菜单里，双卡片保持平齐对称。
- **开放式主题系统**：内置精致暗色与清爽亮色两套，切换时全局 QSS 热重载、Windows 原生标题栏联动明暗、徽章与富文字自动换色，并用 `QSettings` 记住选择。社区只需往 `resources/themes/` 放一个 `.qss` 即可扩展新主题（见 [扩展说明](resources/themes/README.md)），菜单自动挂载。
- **单实例**：重复启动只会把已有窗口置顶；**开机自启动**可在托盘菜单勾选。
- `F5` 刷新、`Esc` 缩回托盘。

### 贡献新语言指南 (Contributing Translations)

想让 StrataTray 说你的语言？**不需要安装编译器、不需要懂 C++**，三步即可：

1. **Fork 本项目**：在 GitHub 上点 Fork，clone 到本地（也可以直接在网页上编辑）。
2. **复制模板**：把 `resources/i18n/en.json` 复制一份，重命名为目标语言，例如 `ja_JP.json`、`ko_KR.json`、`fr_FR.json`。文件名去掉 `.json` 就是语言 id。
3. **翻译并提交 PR**：只改 JSON 里的**值**，不要动键名；把 `_meta.name` 改成该语言的自称（如 `日本語`），其余逐条翻译，提交 PR 即可被合并进官方版本！

```json
{
  "_meta": { "name": "日本語", "locale": "ja_JP" },
  "app_title": "Strata ローカル LLM",
  "start_service": "サービスを開始",
  "stop_service": "サービスを停止",
  "service_status": "サービスの状態"
}
```

> 只翻译一部分也没关系：未提供的词条会**自动回退到 `en.json`**，界面不会出现空白或 key。
> 想立刻预览：把 JSON 放进 `release/i18n/` 再重启程序，子菜单里就会多出该语言（每次构建时 CMake 也会自动把 `resources/i18n/*.json` 复制过去）。
> 请勿改动 `en.json` 的键名；如果确实需要新增界面文案，请在 PR 里一并说明对应的 C++ 调用点。

### 环境要求与构建

- Windows 10 (19041+) / 11；Qt 6.x MSVC 64-bit（CMake 亦可回退 Qt5）
- CMake 3.16+、Visual Studio 2022（MSVC）、Ninja（可选）
- 一份可用的 Strata 本体（含 `stratactl.py`、`update-local.py`）

```bat
build.bat
```

脚本自动用 `vswhere.exe` 定位 VS、探测 Qt（`QTDIR` / `QT_DIR` / PATH / 常见安装路径），配置 `build/`、编译 Release，并调用 `windeployqt` 把运行库部署到 `release/`。产物：`release\StrataTray.exe`（GUI 子系统，无控制台黑框）。

未自动找到 Qt 时：

```bat
set QT_DIR=C:\Qt\6.8.0\msvc2022_64
build.bat
```

手动 CMake：

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
cmake --build build --config Release
"%QT_DIR%\bin\windeployqt.exe" --release release\StrataTray.exe
```

### 配置 Strata 根目录

优先级：`STRATA_PATH` / `STRATA_DIR` → 从 exe 目录向上递归找 `stratactl.py` → 当前工作目录 → 各磁盘根下的 `Strata` 文件夹。

```bat
set STRATA_PATH=D:\MyStrata
```

### 开源协议

本项目采用 [MIT](LICENSE) 协议，欢迎通过 PR 贡献新的语言包与功能改进。

---

## English

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

### Overview

StrataTray is an independent desktop shell for Strata, written in modern C++17 / Qt Widgets. It never modifies Strata: start/stop, version checks and upgrades delegate to Strata's own Python scripts. The code is split into `core/` (config, process probe, update service) and `ui/` (i18n, theme, main window, tray controller), with all Win32-specific code guarded by `#ifdef Q_OS_WIN`.

### Features

- Resident tray + immersive dark UI (native dark title bar and rounded corners on Windows 10/11).
- One-click start / stop with state-aware button labels and colors.
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

### Strata root detection

`STRATA_PATH` / `STRATA_DIR` → walk up from the exe directory looking for `stratactl.py` → current working directory → `Strata` folders on available drives. No machine-specific absolute paths are baked into the source.

### License

Released under the [MIT License](LICENSE). PRs for new language packs and improvements are welcome.

---

### 鸣谢与邀请 / Credits & Invite

> 本项目由 **WorkBuddy (hy4)** 与 **Google Antigravity (Gemini 3.8 Flash)** 深度协作、结对编程配合完成。
> 欢迎使用高效的 AI 结对编程助手 WorkBuddy：[点击获取 WorkBuddy 专属邀请体验](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> 短链：`https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`

> Built through deep pair-programming collaboration between **WorkBuddy (hy4)** and **Google Antigravity (Gemini 3.8 Flash)**.
> Try WorkBuddy, the AI pair-programming assistant: [Get your WorkBuddy invite](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> Short link: `https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`
