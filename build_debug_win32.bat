@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "SOLUTION=%ROOT_DIR%WebFrame.sln"
set "VS_DIR=D:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
set "MSBUILD_EXE=%VS_DIR%\MSBuild\Current\Bin\MSBuild.exe"
set "VCVARS32_BAT=%VS_DIR%\VC\Auxiliary\Build\vcvars32.bat"
set "VSINSTALLDIR=%VS_DIR%\"
set "VCINSTALLDIR=%VS_DIR%\VC\"
set "VCTargetsPath=%VS_DIR%\MSBuild\Microsoft\VC\v160\"
set "VisualStudioVersion=16.0"

if not exist "%MSBUILD_EXE%" (
  echo [ERROR] MSBuild not found: %MSBUILD_EXE%
  exit /b 1
)

if not exist "%VCVARS32_BAT%" (
  echo [ERROR] vcvars32.bat not found: %VCVARS32_BAT%
  exit /b 1
)

if not exist "%SOLUTION%" (
  echo [ERROR] Solution not found: %SOLUTION%
  exit /b 1
)

echo ========================================
echo [BUILD] Debug ^| Win32
echo [BUILD] Solution: %SOLUTION%
echo ========================================

call "%VCVARS32_BAT%" >nul
if errorlevel 1 (
  echo [ERROR] Failed to initialize VS2019 x86 build environment.
  exit /b 1
)

echo [INFO] VS2019 x86 environment ready.
echo [INFO] Starting MSBuild...
"%MSBUILD_EXE%" "%SOLUTION%" /t:Build /p:Configuration=Debug /p:Platform=Win32 /m:1
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
  echo [ERROR] Debug ^| Win32 build failed with exit code %BUILD_EXIT%.
  exit /b %BUILD_EXIT%
)

echo [OK] Debug ^| Win32 build succeeded.
exit /b 0
