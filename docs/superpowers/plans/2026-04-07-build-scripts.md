# Build Scripts Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为当前仓库新增两个可直接执行的 Windows 构建脚本，分别构建 VS2019 下的 `Debug|Win32` 与 `Release|Win32`，并在 README 中补充最小使用说明。

**Architecture:** 方案采用两个根目录独立 `.bat` 文件，分别封装固定的 VS2019 Community 路径、MSBuild 路径与必要环境变量设置，再直接调用 `WebFrame.sln` 的对应配置构建。README 只补充脚本入口，不引入公共脚本、参数化封装或额外自动化层，保持最小改动。

**Tech Stack:** Windows Batch (`.bat`), Visual Studio 2019 MSBuild, existing `WebFrame.sln`, Markdown (`README.md`)

---

## File Structure

- Create: `build_debug_win32.bat` — 固定 VS2019 路径，构建 `Debug|Win32`。
- Create: `build_release_win32.bat` — 固定 VS2019 路径，构建 `Release|Win32`。
- Modify: `README.md` — 增加脚本调用说明，告诉用户从仓库根目录直接运行哪两个脚本。

### Task 1: Add the Debug Win32 build script

**Files:**
- Create: `build_debug_win32.bat`
- Test: `build_debug_win32.bat`

- [ ] **Step 1: Create the Debug build script**

```bat
@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "SOLUTION=%ROOT_DIR%WebFrame.sln"
set "VS_DIR=D:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
set "MSBUILD_EXE=%VS_DIR%\MSBuild\Current\Bin\MSBuild.exe"
set "VSINSTALLDIR=%VS_DIR%\"
set "VCINSTALLDIR=%VS_DIR%\VC\"
set "VCTargetsPath=%VS_DIR%\MSBuild\Microsoft\VC\v160\"
set "VisualStudioVersion=16.0"

if not exist "%MSBUILD_EXE%" (
  echo [ERROR] MSBuild not found: %MSBUILD_EXE%
  exit /b 1
)

if not exist "%SOLUTION%" (
  echo [ERROR] Solution not found: %SOLUTION%
  exit /b 1
)

echo [INFO] Building WebFrame.sln - Debug|Win32
"%MSBUILD_EXE%" "%SOLUTION%" /t:Build /p:Configuration=Debug /p:Platform=Win32 /m:1
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
  echo [ERROR] Build failed with exit code %BUILD_EXIT%.
  exit /b %BUILD_EXIT%
)

echo [INFO] Build succeeded.
exit /b 0
```

- [ ] **Step 2: Run the script to verify it starts a Debug build**

Run:

```bat
build_debug_win32.bat
```

Expected:

- Console prints `Building WebFrame.sln - Debug|Win32`
- MSBuild starts the `Debug|Win32` build
- Script exits with `0` on success, non-zero on failure

- [ ] **Step 3: Re-run to verify exit code behavior from cmd**

Run:

```bat
cmd /c build_debug_win32.bat & echo %ERRORLEVEL%
```

Expected:

- Final line is `0` when the build succeeds

- [ ] **Step 4: Commit**

```bash
git add build_debug_win32.bat
git commit -m "build: add Debug Win32 build script"
```

### Task 2: Add the Release Win32 build script

**Files:**
- Create: `build_release_win32.bat`
- Test: `build_release_win32.bat`

- [ ] **Step 1: Create the Release build script**

```bat
@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "SOLUTION=%ROOT_DIR%WebFrame.sln"
set "VS_DIR=D:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
set "MSBUILD_EXE=%VS_DIR%\MSBuild\Current\Bin\MSBuild.exe"
set "VSINSTALLDIR=%VS_DIR%\"
set "VCINSTALLDIR=%VS_DIR%\VC\"
set "VCTargetsPath=%VS_DIR%\MSBuild\Microsoft\VC\v160\"
set "VisualStudioVersion=16.0"

if not exist "%MSBUILD_EXE%" (
  echo [ERROR] MSBuild not found: %MSBUILD_EXE%
  exit /b 1
)

if not exist "%SOLUTION%" (
  echo [ERROR] Solution not found: %SOLUTION%
  exit /b 1
)

echo [INFO] Building WebFrame.sln - Release|Win32
"%MSBUILD_EXE%" "%SOLUTION%" /t:Build /p:Configuration=Release /p:Platform=Win32 /m:1
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
  echo [ERROR] Build failed with exit code %BUILD_EXIT%.
  exit /b %BUILD_EXIT%
)

echo [INFO] Build succeeded.
exit /b 0
```

- [ ] **Step 2: Run the script to verify it starts a Release build**

Run:

```bat
build_release_win32.bat
```

Expected:

- Console prints `Building WebFrame.sln - Release|Win32`
- MSBuild starts the `Release|Win32` build
- Script exits with `0` on success, non-zero on failure

- [ ] **Step 3: Re-run to verify exit code behavior from cmd**

Run:

```bat
cmd /c build_release_win32.bat & echo %ERRORLEVEL%
```

Expected:

- Final line is `0` when the build succeeds

- [ ] **Step 4: Commit**

```bash
git add build_release_win32.bat
git commit -m "build: add Release Win32 build script"
```

### Task 3: Document the script usage in README

**Files:**
- Modify: `README.md:50-68`
- Test: `README.md`

- [ ] **Step 1: Update the build section with script usage**

Insert the following block under the `## 构建与运行` section before `### 构建依赖`:

```md
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
```

- [ ] **Step 2: Read README to verify the inserted section is correct**

Run:

```text
Read README.md and confirm the new section appears under `## 构建与运行`.
```

Expected:

- The new `### 快速构建脚本` section is present
- The two script names are correct
- The VS2019 path matches `D:\Program Files (x86)\Microsoft Visual Studio\2019\Community`

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: add Win32 build script usage"
```

### Task 4: Final verification

**Files:**
- Test: `build_debug_win32.bat`
- Test: `build_release_win32.bat`
- Test: `README.md`

- [ ] **Step 1: Run the Debug script from the repository root**

Run:

```bat
build_debug_win32.bat
```

Expected:

- `Debug|Win32` build starts and completes
- Script exits with `0`

- [ ] **Step 2: Run the Release script from the repository root**

Run:

```bat
build_release_win32.bat
```

Expected:

- `Release|Win32` build starts and completes
- Script exits with `0`

- [ ] **Step 3: Verify output artifacts still exist after script-based builds**

Run:

```bat
dir bin\Debug\box.exe
dir bin\Release\box.exe
```

Expected:

- Both commands find `box.exe`

- [ ] **Step 4: Commit**

```bash
git add build_debug_win32.bat build_release_win32.bat README.md
git commit -m "build: add Win32 build scripts"
```
