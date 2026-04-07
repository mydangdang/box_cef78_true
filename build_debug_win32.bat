@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "SOLUTION=%ROOT_DIR%WebFrame.sln"
set "VS_DIR=D:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
set "MSBUILD_EXE=%VS_DIR%\MSBuild\Current\Bin\MSBuild.exe"
set "VCVARS32_BAT=%VS_DIR%\VC\Auxiliary\Build\vcvars32.bat"
set "BUILD_EXIT=0"

echo ========================================
echo [BUILD] Debug ^| Win32
echo [BUILD] Solution: %SOLUTION%
echo ========================================
echo.

if not exist "%MSBUILD_EXE%" goto err_msbuild
if not exist "%VCVARS32_BAT%" goto err_vcvars
if not exist "%SOLUTION%" goto err_solution

call "%VCVARS32_BAT%"
if errorlevel 1 goto err_env

echo.
echo [INFO] VS2019 x86 environment ready.
echo [INFO] Starting MSBuild...
echo.
"%MSBUILD_EXE%" "%SOLUTION%" /t:Build /p:Configuration=Debug /p:Platform=Win32 /m:1
set "BUILD_EXIT=%ERRORLEVEL%"
if not "%BUILD_EXIT%"=="0" goto err_build

echo.
echo [OK] Debug ^| Win32 build succeeded.
goto done

:err_msbuild
echo [ERROR] MSBuild not found:
echo %MSBUILD_EXE%
set "BUILD_EXIT=1"
goto done

:err_vcvars
echo [ERROR] vcvars32.bat not found:
echo %VCVARS32_BAT%
set "BUILD_EXIT=1"
goto done

:err_solution
echo [ERROR] Solution not found:
echo %SOLUTION%
set "BUILD_EXIT=1"
goto done

:err_env
echo [ERROR] Failed to initialize VS2019 x86 build environment.
set "BUILD_EXIT=1"
goto done

:err_build
echo.
echo [ERROR] Debug ^| Win32 build failed with exit code %BUILD_EXIT%.
goto done

:done
echo.
pause
exit /b %BUILD_EXIT%
