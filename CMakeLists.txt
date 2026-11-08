cmake_minimum_required(VERSION 3.16)
project(StrataTray LANGUAGES CXX)

# ---- Qt 自动化：moc / rcc / uic（.h 中的 Q_OBJECT 由 AUTOMOC 处理）----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- C++17 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 依赖：优先 Qt6，找不到时自动回退 Qt5 ----
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# ---- Windows 资源编译器：把 resources/app.ico 编进 exe ----
if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"")
endif()

# ---- 多文件模块化：递归收集 src/ 下的源码与头文件 ----
file(GLOB_RECURSE APP_CPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE APP_H   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
set(APP_SOURCES ${APP_CPP} ${APP_H} resources/resources.qrc)
if(WIN32)
    list(APPEND APP_SOURCES resources/app.rc)
endif()

# WIN32 子系统：运行时不弹出黑框控制台
add_executable(StrataTray WIN32 ${APP_SOURCES})
target_include_directories(StrataTray PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(StrataTray PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# MSVC：源码含中文，强制按 UTF-8 解析
if(MSVC)
    target_compile_options(StrataTray PRIVATE /utf-8)
endif()

if(WIN32)
    # dwmapi: 暗色标题栏 / 圆角；iphlpapi: 原生 TCP 监听表查询
    target_link_libraries(StrataTray PRIVATE dwmapi iphlpapi)
endif()

# ---- 多语言包：构建后复制到 release/i18n/ ----
# 贡献者新增 resources/i18n/xx_YY.json 后无需修改 qrc 或任何 C++ 代码，
# 程序启动时会动态发现该目录里的语言包并挂到二级子菜单。
file(GLOB I18N_JSON CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n/*.json")
if(I18N_JSON)
    add_custom_command(TARGET StrataTray POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/release/i18n"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${I18N_JSON} "${CMAKE_SOURCE_DIR}/release/i18n/"
        COMMENT "Deploying i18n language packs")
endif()

# 输出目录固定为 release/，windeployqt 与桌面快捷方式都指向这里
set_target_properties(StrataTray PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/release"
)
