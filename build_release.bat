@echo off
echo Building Real-Time Game Engine (RELEASE)...

REM Check for command line arguments
set LOGGING=OFF
if "%1"=="log" (
    set LOGGING=ON
    echo Logging is ENABLED
) else (
    echo Logging is DISABLED
)

if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_C_STANDARD=11 -DENABLE_LOGGING=%LOGGING% ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b %ERRORLEVEL%
)

cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo Run the engine with: .\Release\RealTimeGameEngine.exe
cd .. 