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
└── ui/
    ├── I18n.h/.cpp             # 轻量中英语言管理（默认跟随系统 locale）
    ├── Theme.h                 # QSS 主题与高 DPI 图标
    ├── MainWindow.h/.cpp       # 双卡片布局、Tab、状态刷新、按钮信号
    └── TrayApp.h/.cpp          # 托盘、菜单、IPC 单实例、Core/UI 调度
```

平台相关代码（`dwmapi.h`、`iphlpapi.h`、注册表）严格包在 `#ifdef Q_OS_WIN` 中，其它平台提供合理 Fallback。

### 主要特性

- **常驻托盘 + 沉浸式暗色界面**：Win10/Win11 原生暗色标题栏与圆角，托盘图标随状态变色。
- **一键启动 / 停止**（停止释放显存），启动中 / 就绪 / 异常有明确按钮文案与颜色引导。
- **一键复制 OpenAI 兼容接口** `/v1`（或 Base URL），带「已复制 ✓」绿色反馈。
- **显存与日志实时流**：微型显存条、最后 200 行日志（只读末尾 64 KB，日志再大也不卡）、错误/警告着色、自动滚动、清空视图、打开日志文件。
- **版本对比与一键热更新**：本地 / 线上版本号、官方发布要点、落后提交数、5 步进度与耗时，完成后自动复核版本。
- **中英双语**：默认跟随系统语言；主窗口右上角「🌐 English / 🌐 中文」按钮或托盘菜单「切换到 English / 切换到中文」一键切换，两处状态联动，无需重启。
- **单实例**：重复启动只会把已有窗口置顶；**开机自启动**可在托盘菜单勾选。
- `F5` 刷新、`Esc` 缩回托盘。

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

发布前请补充 `LICENSE`（推荐 MIT）与 `CONTRIBUTING.md`。

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
- **Bilingual UI (中文 / English)**: follows the system locale by default; switch instantly via the **🌐 English / 🌐 中文** button in the window's top-right corner or the tray menu, both stay in sync, no restart.
- Single instance (second launch raises the existing window) and optional run-at-startup.
- `F5` refresh, `Esc` hide to tray.

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

Add a `LICENSE` (MIT recommended) before publishing.

---

### 鸣谢与邀请 / Credits & Invite

> 本项目由 **WorkBuddy (hy4)** 与 **Google Antigravity (Gemini 3.8 Flash)** 深度协作、结对编程配合完成。
> 欢迎使用高效的 AI 结对编程助手 WorkBuddy：[点击获取 WorkBuddy 专属邀请体验](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> 短链：`https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`

> Built through deep pair-programming collaboration between **WorkBuddy (hy4)** and **Google Antigravity (Gemini 3.8 Flash)**.
> Try WorkBuddy, the AI pair-programming assistant: [Get your WorkBuddy invite](https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92)
> Short link: `https://www.workbuddy.cn/events/invite?inviteCode=a11iin3afilcl92`
