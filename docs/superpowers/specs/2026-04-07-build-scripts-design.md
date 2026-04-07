# Windows Build Scripts Design

**Goal:** 为当前仓库提供两个可直接执行的 Windows 构建脚本，分别用于 VS2019 下的 `Debug|Win32` 和 `Release|Win32` 构建。

## Background

当前仓库已经完成以下构建前提：

- `WebFrame.sln` 已调整为 VS2019 兼容格式。
- `tests/cefclient/cefclient.vcxproj` 与 `libcef_dll_wrapper/libcef_dll_wrapper.vcxproj` 已切换到 `v142` 工具集。
- 已验证在指定 VS2019 安装路径下可以完成 `Release|Win32` 构建。
- 用户希望后续能够通过脚本直接执行构建，而不必每次手动拼接环境变量和 MSBuild 命令。

已确认的 Visual Studio 2019 路径为：

- `D:\Program Files (x86)\Microsoft Visual Studio\2019\Community`

## Chosen Approach

在仓库根目录新增两个独立的 `.bat` 文件：

- `build_debug_win32.bat`
- `build_release_win32.bat`

每个脚本都独立完成自己的配置，不共享额外公共脚本。

## Why This Approach

这是当前需求下最小、最直接、最稳定的方案：

- 双击即可执行。
- 命令行也可直接调用。
- 不需要用户记忆参数。
- 不引入额外抽象层或公共脚本。
- 与当前仓库“就地可用”的目标一致。

相比单脚本传参或 PowerShell 方案，这种方式对后续使用者更直观，也更适合当前项目的手工构建场景。

## Script Behavior

两个脚本将执行相同的流程，只在 `Configuration` 值上不同：

1. 定位仓库根目录下的 `WebFrame.sln`。
2. 固定使用以下 MSBuild 路径：
   - `D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe`
3. 固定设置以下环境变量：
   - `VSINSTALLDIR`
   - `VCINSTALLDIR`
   - `VCTargetsPath`
   - `VisualStudioVersion`
4. 调用 `MSBuild.exe` 执行构建：
   - Debug 脚本使用 `Configuration=Debug; Platform=Win32`
   - Release 脚本使用 `Configuration=Release; Platform=Win32`
5. 在控制台输出成功或失败结果。
6. 返回正确的 `%ERRORLEVEL%`，便于后续命令行或自动化流程复用。

## Files To Add

- `build_debug_win32.bat`
- `build_release_win32.bat`

## Files To Update

- `README.md`

更新 README 的目的仅限于补充脚本用法，避免用户以后不知道如何直接调用。

## Out of Scope

本次不包含以下内容：

- 不新增公共脚本或脚本库。
- 不新增 PowerShell 版本。
- 不修改现有 `.sln` / `.vcxproj` 构建逻辑。
- 不处理 `app.txt` 业务入口逻辑。
- 不扩展到 x64 或其他配置。

## Success Criteria

以下条件满足即可视为完成：

1. 根目录存在两个可执行的 `.bat` 文件。
2. 运行 `build_debug_win32.bat` 可触发 `Debug|Win32` 构建。
3. 运行 `build_release_win32.bat` 可触发 `Release|Win32` 构建。
4. 构建成功时脚本返回 `0`。
5. 构建失败时脚本返回非 `0`。
6. README 中包含简短的脚本使用说明。

## Risks

主要风险只有一个：

- 脚本固定绑定到当前机器确认可用的 VS2019 安装路径。如果未来 Visual Studio 被移动、重装或版本切换，脚本路径需要同步更新。

这是当前需求可接受的取舍，因为用户已经明确要求基于这个已确认可用的路径来做脚本。
