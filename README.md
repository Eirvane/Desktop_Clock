<p align="center">
  <img src="https://raw.githubusercontent.com/Eirvane/Moment/main/images/Moment.png" width="150" alt="Moment logo">
</div>

<h1 align="center">Moment - 桌面时钟    一款运行与Windows的桌面时钟应用</h1>

___

<p align="center"> <b>支持显示当前时间，正向计时，自定义倒计时</b> </p>

<p align="center"> 
<a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-yellow" alt="License"/></a>
<a href="https://en.cppreference.com/w/c"><img src="https://img.shields.io/badge/C-A8B9CC?logo=c&logoColor=black" alt="C"/></a>
<a href="https://cmake.org/"><img src="https://img.shields.io/badge/CMake-064F8C?logo=cmake&logoColor=white" alt="CMake"/></a>
<a href="https://github.com/Eirvane/Moment/releases"><img src="https://img.shields.io/github/v/release/Eirvane/Moment?color=green&label=Release" alt="Release"/></a>
</p>

## ✨简介

___

**桌面时钟 (Moment)** 一款专为 Windows 设计的极简桌面时钟。不仅支持实时显示北京时间，还内置了灵活的正向/倒计时模式。你可以自由调整字体大小、颜色及窗口置顶状态，所有设置均可通过系统托盘右键快速访问，让时间管理既美观又高效。

>应用体积极小，仅43.1 MB大小。运行时占用内存25.4MB，静默运行无干扰，对任何设备占用极低

## 📋图形界面预览
___
<p align="center">
  <td align="center"><b>时钟主界面（中间）</b><br/><img src="https://raw.githubusercontent.com/Eirvane/Moment/main/images/Main_Window.png" width="400" alt="Main Windown">
</div>
<p align="center">
  <td align="center"><b>托盘设置主界面</b><br/><img src="https://raw.githubusercontent.com/Eirvane/Moment/main/images/UI.png" width="150" alt="Set UI">
</div>

**界面清爽简洁，专注于时间设置，没有任何多余的功能**

## 📌功能及使用方法
___
- **模式**        更换主窗口的显示模式，支持切换 **当前时间** 、**正计时** 、**倒计时**
    - **当前时间** ：应用默认显示当前的北京时间（UTC +8:00）
    - **正计时** ：切换后点击开始，主窗口开始从0秒开始正向计时
    - **倒计时** ：切换后选择需要的时间范围，点击开始，时间倒向计时（当前暂无结束提醒）

- **置顶**       自定义选择主窗口是否置顶，点击切换右侧
    - “置顶”前显示**√**代表当前为置顶状态。置顶状态下，主窗口不会被任何窗口覆盖，始终位于你的界面最上方
    - “置顶”前无**√** 代表当前状态为非置顶状态。置顶状态下，主窗口只显示在桌面壁纸及应用图标之上，能够被其他窗口覆盖，以免遮盖你正在使用的窗口

- **已固定/正在移动**         默认状态为已固定，点击一次切换为正在移动
    - **已固定** ：主窗口锁定，取消对鼠标信息的读取，无法对主窗口进行更改
    - **正在移动** ：主窗口取消锁定，出现低透明度窗口及边框，此时可移动主窗口。鼠标停悬在窗口上方滚动滚轮可对数字大小进行更改

- **字体**      设置时钟主窗口的数字字体显示

   内置字体``Abel-Regular-2.ttf`` ``BebasNeue-1.otf`` ``BebasNeue-1.otf`` ``DS-DIGIB.ttf`` ``DS-DIGIT.ttf`` ``NationalPark-Regular.otf`` 
     ``OPPO Sans.ttf`` ``Pixelify Sans Medium Essence.ttf`` ``Rousseau-Deco.ttf`` ``SC Regular.ttf``
     
     添加字体可在二级窗口点击**打开字体文件夹** ，将需要添加的字体拖进/复制进该目录

- **颜色**      更改时钟主窗口数字的颜色
        支持**颜色面板**自选以及**颜色代码**的设置   **注：颜色值已默认敲入#号，只需要输入数字即可**
    
- **显示**      修改时间的时值
        选择主窗口时间显示为**24小时制**或**12小时制** ；自定义是否显示时间后的秒数

- **退出**      退出程序

正在开发的功能：开机自启动........

## 🧱 技术栈
---

| 层     | 技术                                                                            |
| ----- | ----------------------------------------------------------------------------- |
| 桌面外壳  | Win32 原生 API（`CreateWindowExW` / 分层窗口 `WS_EX_LAYERED`）                        |
| 实现语言  | C11（界面/窗口逻辑） + C++17（GDI+ 渲染器 `renderer.cpp`）                                 |
| 渲染    | GDI+（`Bitmap` / `Graphics` / `PrivateFontCollection` 私有字体）                    |
| 窗口样式  | DWM API（`DwmSetWindowAttribute` 圆角 / 亚克力效果）                                   |
| 交互    | 托盘通知图标 `Shell_NotifyIcon` + 自绘弹出菜单                                            |
| 配置持久化 | INI 文件 `clock.ini`（`GetPrivateProfileStringW` / `WritePrivateProfileStringW`） |
| 计时核心  | `GetTickCount64` + `SetTimer` 1s 轮询（时钟 / 正计时 / 倒计时三模式）                        |
| 图标资源  | `resources.rc` 资源脚本（图标 `IDI_APP_ICON`）                                        |
| 构建系统  | CMake 3.15+（Visual Studio 2026 生成器，MSVC 19.51，x64）                            |
| 编码规范  | Unicode 全程 `W` 后缀 API + `/utf-8` 编译选项                                         |
| 链接库   | `gdiplus` `ole32` `shell32` `dwmapi` `comdlg32` `uuid` 等系统库                   |
| 部署形态  | 单文件绿色 EXE（依赖同目录 `fonts/` 字体文件夹，无安装器）                                          |

## 🚀 快速构建
--- 
#####  所需工具

| 工具        | 作用      |         下载链接                                                                   |
| --------- | ------- | ------------------------------------------------------------------------------ |
| **MinGW** | GCC 编译器 | [MinGW Build](https://github.com/niXman/mingw-builds-binaries/releases/latest) |
| **CMake** | 构建工具    | [CMake](https://cmake.org/download/)                                           |
##### 本地构建步骤
```bash
# 在 build 目录中生成 x64 架构的项目文件
cmake -B build -A x64

# 执行构建，编译 build 目录中的项目并生成 Release 配置的可执行文件/库
cmake --build build --config Release
```

> 环境要求：Node.js ≥ 18、Windows 10/11

## 🗂️ 项目结构
---

```
Desktop_Clock/
├── src/
│   ├── core/
│   │   └── main.c                程序入口：COM/GDI+ 初始化、DPI 感知、主消息循环
│   ├── platform/
│   │   ├── window.c/h            主窗口：分层窗口创建 / 托盘消息分发 / 菜单命令
│   │   │                       （移动拖拽、滚轮缩放、置顶/固定切换、菜单钩子）
│   │   └── trayicon.c/h          托盘图标：Shell_NotifyIcon 注册/销毁
│   ├── graphics/
│   │   └── renderer.cpp/h        GDI+ 渲染器：时间/正计时/倒计时绘制
│   │                           （私有字体加载、边框框架、透明位图输出）
│   ├── ui/
│   │   ├── fontmenu.c/h          字体子菜单：扫描 fonts/ 目录 / 切换字体
│   │   ├── colorpicker.c/h       颜色设置：HEX 输入框 / 系统颜色面板
│   │   └── countdown.c/h         模式对话框：倒计时设置 / 正计时启动
│   └── utils/
│       ├── config.c/h            INI 配置：clock.ini 读写 / 默认值
│       └── utils.c/h             通用工具：UI 字体 / 圆角窗口 / 颜色解析
├── resources.rc                  图标资源（IDI_APP_ICON）
├── resource.h                    资源 ID 定义
└── CMakeLists.txt                CMake 构建脚本
```


## 📦 下载
---
- 前往 [**Releases**](https://github.com/Eirvane/Moment/releases)下载最新 Windows 安装包。


## 🥇支持
---
**特别感谢[Tsukiyig](https://github.com/Tsukiyig)提供的图标设计**

<div align="left">
  <a href="https://github.com/Tsukiyig">
    <img src="https://avatars.githubusercontent.com/u/325215227?v=4" width="100" />
  </a>
</div>

[Tsukiyig](https://github.com/Tsukiyig)

**本项目大量源代码均为AI生成**
