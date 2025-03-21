@echo off
echo Setting up dependencies for 3D Model Viewer...
echo.

REM Check if vcpkg is installed
where vcpkg > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo vcpkg is not found in your PATH.
    echo.
    echo You need to install vcpkg first:
    echo 1. Clone the repository: git clone https://github.com/microsoft/vcpkg
    echo 2. Run bootstrap: .\vcpkg\bootstrap-vcpkg.bat
    echo 3. Add vcpkg to your PATH
    echo.
    echo Press any key to open the vcpkg GitHub page...
    pause > nul
    start "" "https://github.com/microsoft/vcpkg"
    exit /b 1
)

echo Found vcpkg in your PATH. Proceeding with installation...
echo.

REM Detect system architecture
if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
    set ARCH=x64-windows
) else (
    set ARCH=x86-windows
)
echo Detected architecture: %ARCH%
echo.

echo Installing OpenGL dependencies...
vcpkg install glew:%ARCH%
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLEW.
    pause
    exit /b 1
)

echo Installing Assimp...
vcpkg install assimp:%ARCH%
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install Assimp.
    pause
    exit /b 1
)

echo All dependencies installed successfully!
echo.

REM Generate CMake command for model viewer
set CMAKE_CMD=cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ..

echo To use these dependencies in your project:
echo 1. Make sure to set the VCPKG_ROOT environment variable to your vcpkg installation directory
echo 2. Configure CMake with: %CMAKE_CMD%
echo 3. Build with: cmake --build . --target model_viewer
echo.
echo Press any key to run the dependency checker...
pause > nul

REM Run the dependency checker if it exists
set CHECKER=build\bin\Debug\dependency_checker.exe
if exist %CHECKER% (
    start "" %CHECKER%
) else (
    echo Dependency checker not found. Build it first with:
    echo cmake --build . --target dependency_checker
)

echo.
echo Setup complete. Press any key to exit...
pause > nul 