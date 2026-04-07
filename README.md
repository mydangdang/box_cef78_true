# box_cef78_true

一个基于 CEF 78 的 Windows 桌面壳工程，用于承载本地或远程 HTML 页面，并通过 JS 与 Native 桥接调用宿主能力。

## 项目概览

- 解决方案：[WebFrame.sln](WebFrame.sln)
- 主工程：[tests/cefclient/cefclient.vcxproj](tests/cefclient/cefclient.vcxproj)
- 输出程序：`box.exe`
- 技术栈：CEF 78、C++、Win32、Visual Studio/MSBuild、本地 HTML/CSS/JS

当前工程明显基于 CEF 官方 `cefclient` 样例扩展而来，并加入了窗口控制、浏览器子窗口、系统能力、热键、录屏、闹钟等业务能力。

## 目录结构

- [WebFrame.sln](WebFrame.sln)  
  Visual Studio 解决方案入口。
- [tests/cefclient/](tests/cefclient/)  
  主程序源码、资源、业务逻辑与 Windows 宿主实现。
- [tests/shared/](tests/shared/)  
  来自 CEF 示例的共享代码。
- [include/](include/)  
  CEF 头文件。
- [libcef_dll/](libcef_dll/)  
  CEF 相关源码目录。
- [libcef_dll_wrapper/](libcef_dll_wrapper/)  
  CEF wrapper 工程与代码。
- [cefdepends/](cefdepends/)  
  外部依赖目录，包含 include、运行资源和附加组件来源。
- [webtest/](webtest/)  
  本地前端示例页面与静态资源。
- [app.txt](app.txt)  
  运行时窗口与入口 URL 配置。

## 编译环境

根据当前工程配置，建议使用以下环境：

- Visual Studio 2019 打开解决方案
- C++ `v142` 工具集
- Windows SDK `10.0.19041.0`（或安装的兼容 10.0 SDK）
- 目标平台：`Win32`

说明：

- `WebFrame.sln` 已调整为 VS2019 格式，工程文件使用 `v142` 工具集。
- 解决方案里虽然能看到 `x64` 配置名，但当前工程实际仍映射到 `Win32`。
- 输出文件名不是 `cefclient.exe`，而是 `box.exe`。
- 原工程对 ATL Server 的 `atlrx.h` 有依赖；仓库现已在 `tests/cefclient/atlrx.h` 提供最小兼容实现，避免 VS2019 环境缺失该头文件。

## 构建与运行

### 快速构建脚本

仓库根目录提供两个可直接执行的 Windows 构建脚本：

- `build_debug_win32.bat`
- `build_release_win32.bat`

用法：

```bat
build_debug_win32.bat
build_release_win32.bat
```

说明：

- 两个脚本固定使用 `D:\Program Files (x86)\Microsoft Visual Studio\2019\Community` 下的 VS2019/MSBuild。
- `build_debug_win32.bat` 构建 `Debug|Win32`。
- `build_release_win32.bat` 构建 `Release|Win32`。
- 构建失败时脚本会返回非 `0` 退出码。

### 构建依赖

主工程会使用这些依赖路径：

- `cefdepends/include`
- `cefdepends/<Config>/lib`
- `include/`
- `libcef_dll_wrapper/`

构建后还会通过 PostBuild 复制以下运行时资源到输出目录：

- `cefdepends/<Config>/bin`
- `cefdepends/Resources`
- `cefdepends/addtions`

因此本仓库目前是“尽量保留可编译/可运行所需依赖”的策略，没有做激进二进制清理。

### 运行入口与 app.txt

程序启动时，最终会优先读取当前目录下的 [app.txt](app.txt) 来确定入口页面和窗口参数。

当前 `app.txt` 内容指向的是开发机绝对路径：

- `E:/MyNAS/WebFrame/webtest/index.html`

这不是通用路径，新机器直接运行前需要先修改。

如果要使用仓库自带的本地页面，建议把 `app.txt` 中的 `url` 改成可访问的仓库内页面路径，例如：

- `webtest/index.html`

当前代码还存在远端配置拉取逻辑，但只要本地 `app.txt` 可读，通常会优先采用本地配置。

## 前端桥接

本地示例页面位于：

- [webtest/index.html](webtest/index.html)
- [webtest/resource/js/main.js](webtest/resource/js/main.js)

其中 `main.js` 通过 `window.nativeCall(...)` 与 Native 交互，示例覆盖了：

- 窗口控制
- Chrome/IE 子窗口管理
- 广播消息
- 本地/内存存储
- 热键
- 截图与录屏
- 闹钟与定时关机
- 硬件信息

## 仓库说明

- 当前工作分支已切到 `main`。
- 远程 `master` 仍保留，作为回退分支。
- 第一轮仓库整理只移除了低争议噪声文件：IDE 元数据、日志、资源编辑缓存。
- 两个超大 CEF zip 包没有纳入版本库。

## 已知注意事项

当前仓库中有两个值得注意的问题：

1. 工程引用了 `AtlServer/include`，但仓库里未看到对应目录。  
2. 启动画面逻辑依赖 `staticres/init.png`，当前仓库里未看到该路径。  

如果后续要做真正的“新环境可完整编译/可完整运行”治理，建议单独再做一轮依赖补齐与路径整理。
