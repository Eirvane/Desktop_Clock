# ============================================================
# DesktopClock - 透明桌面时钟
# 构建系统：CMake 3.16+
# 平台：Windows 10 / 11 (MSVC / MinGW)
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(DesktopClock LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# 强制 Unicode 字符集
add_compile_definitions(UNICODE _UNICODE)

# -----------------------------------------------------------
# 【关键】MSVC 强制静态链接运行时库 (/MT)，避免 VCRUNTIME140.dll 依赖
# 必须在 add_executable 之前
# -----------------------------------------------------------
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# -----------------------------------------------------------
# 源文件
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

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# -----------------------------------------------------------
# 头文件搜索路径
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
# -----------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    gdiplus
    ole32
    user32
    kernel32
    gdi32
    dwmapi
    shell32
    comdlg32
)

# -----------------------------------------------------------
# 安装规则
# -----------------------------------------------------------

# 安装主程序
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# 安装字体文件夹
install(DIRECTORY ${CMAKE_SOURCE_DIR}/fonts/
    DESTINATION fonts
    FILES_MATCHING 
        PATTERN "*.ttf"
        PATTERN "*.otf"
        PATTERN "*.ttc"
        PATTERN "*.fon"
)

# 安装可选文件
install(FILES
    ${CMAKE_SOURCE_DIR}/LICENSE.txt
    ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION .
    OPTIONAL
)

# 安装默认配置文件
if(EXISTS ${CMAKE_SOURCE_DIR}/clock.ini)
    install(FILES ${CMAKE_SOURCE_DIR}/clock.ini
        DESTINATION .
    )
endif()

# -----------------------------------------------------------
# CPack 打包配置
# -----------------------------------------------------------
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "DesktopClock")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "YourName")
include(CPack)