@echo off
setlocal EnableDelayedExpansion
rem ============================================================
rem  StrataTray one-click build (CMake + MSVC + Qt 6.x)
rem  Pure ASCII: cmd.exe mangles UTF-8 batch files on Chinese Windows.
rem ============================================================
cd /d "%~dp0"

rem ---- 1. Locate Qt 6: QTDIR / QT_DIR / PATH / common installs ----
set "QT_ROOT="
if defined QTDIR if exist "%QTDIR%\lib\cmake\Qt6\Qt6Config.cmake" set "QT_ROOT=%QTDIR%"
if not defined QT_ROOT if defined QT_DIR if exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" set "QT_ROOT=%QT_DIR%"

if not defined QT_ROOT (
  for /f "delims=" %%Q in ('where qmake 2^>nul') do (
    if not defined QT_ROOT (
      for %%D in ("%%~dpQ..") do if exist "%%~fD\lib\cmake\Qt6\Qt6Config.cmake" set "QT_ROOT=%%~fD"
    )
  )
)

if not defined QT_ROOT (
  for %%R in ("C:\Qt" "D:\Qt" "E:\Qt" "F:\Qt") do (
    if exist "%%~R" (
      for /d %%V in ("%%~R\6.*") do (
        if exist "%%~fV\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" set "QT_ROOT=%%~fV\msvc2022_64"
        if not defined QT_ROOT if exist "%%~fV\msvc2019_64\lib\cmake\Qt6\Qt6Config.cmake" set "QT_ROOT=%%~fV\msvc2019_64"
      )
    )
  )
)

if not defined QT_ROOT (
  echo [ERROR] Qt 6.x MSVC 64-bit was not found.
  echo.
  echo   Set it manually, then run build.bat again:
  echo       set QT_DIR=C:\Qt\6.8.0\msvc2022_64
  echo.
  pause
  exit /b 1
)
echo [INFO] Qt: %QT_ROOT%

rem ---- 2. Locate Visual Studio via vswhere, then load vcvars64 ----
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
if exist "%VSWHERE%" (
  for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
    if not defined VSINSTALL set "VSINSTALL=%%I"
  )
)
if defined VSINSTALL if exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" (
  echo [INFO] MSVC: %VSINSTALL%
  call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
) else (
  echo [WARN] vcvars64.bat not found; assuming MSVC env is already loaded.
)

rem ---- 3. Configure + build into build/ ----
set "PATH=%QT_ROOT%\bin;%PATH%"
where ninja >nul 2>nul
if errorlevel 1 (
  echo [INFO] Ninja not found, using CMake default generator.
  cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_ROOT%"
) else (
  echo [INFO] Using Ninja generator.
  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_ROOT%"
)
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  pause
  exit /b 1
)

cmake --build build --config Release
if errorlevel 1 (
  echo [ERROR] Build failed.
  pause
  exit /b 1
)

rem ---- 4. Deploy Qt runtime into release/ ----
echo [INFO] Deploying Qt runtime ...
"%QT_ROOT%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw "release\StrataTray.exe"
if errorlevel 1 echo [WARN] windeployqt returned non-zero; see output above.

echo.
echo ==== Done: %~dp0release\StrataTray.exe ====
pause
endlocal
