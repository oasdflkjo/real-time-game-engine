@echo off
echo Building Real-Time Game Engine...

if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_C_STANDARD=11 ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b %ERRORLEVEL%
)

cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo Run the engine with: .\Debug\RealTimeGameEngine.exe
cd .. 