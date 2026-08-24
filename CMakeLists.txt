# ============================================================
# DesktopClock - 透明桌面时钟
# 构建系统：CMake 3.16+
# 平台：Windows 10 / 11 (MSVC / MinGW)
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(DesktopClock LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# 强制 Unicode 字符集，与源码中的 #define UNICODE 保持一致
add_compile_definitions(UNICODE _UNICODE)

# -----------------------------------------------------------
# 源文件（按模块组织路径）
# -----------------------------------------------------------
set(SOURCES
    src/core/main.c
    src/platform/window.c
    src/platform/trayicon.c
    src/graphics/renderer.cpp
    src/ui/colorpicker.c
    src/ui/fontmenu.c
    src/utils/config.c
    src/utils/utils.c
)

# WIN32：GUI 子系统，不弹出控制台黑框
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# -----------------------------------------------------------
# 头文件搜索路径
# 配置后，所有 .c/.cpp 中只需写 #include "xxx.h"，
# 无需关心 xxx.h 实际位于哪个子目录。
# -----------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    src/core
    src/platform
    src/graphics
    src/ui
    src/utils
)

# -----------------------------------------------------------
# 链接 Windows 系统库
# 各模块中的 #pragma comment(lib, ...) 是 MSVC 专用语法，
# 此处显式链接以确保跨编译器兼容（如 MinGW）。
# -----------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    gdiplus      # GDI+ 渲染
    ole32        # COM 公寓线程
    user32       # 窗口消息
    kernel32     # 核心 API
    gdi32        # 字体加载 / GDI
    dwmapi       # DWM 圆角 / 桌面合成
    shell32      # 系统托盘
    comdlg32     # 颜色选择对话框
)