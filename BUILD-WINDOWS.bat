@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install Visual Studio with the Desktop development with C++ workload
    echo or install CMake and a compatible Windows C compiler.
    exit /b 1
)

cmake -S "%~dp0." -B "%~dp0build" -A x64
if errorlevel 1 exit /b 1

cmake --build "%~dp0build" --config Release
if errorlevel 1 exit /b 1

echo.
echo Build complete: %~dp0build\bin\Release\NICEquipmentManager.exe
endlocal
