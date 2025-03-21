@echo off
setlocal enabledelayedexpansion

REM Detect Visual Studio installation
set VCVARS=
for %%p in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist "%%p" (
        set VCVARS="%%p"
        goto :found_vcvars
    )
)

:found_vcvars
if "!VCVARS!" == "" (
    echo Visual Studio developer environment not found.
    echo Please install Visual Studio with C++ development tools.
    pause
    exit /b 1
)

echo Found Visual Studio at: !VCVARS!
echo Setting up developer environment...
call !VCVARS!

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

cd build

REM Configure with CMake
echo Configuring CMake with detailed output...
cmake -DCMAKE_VERBOSE_MAKEFILE=ON .. 2> cmake_errors.txt

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed. See cmake_errors.txt for details.
    notepad cmake_errors.txt
    cd ..
    pause
    exit /b 1
)

REM Build the simple_paint application with detailed output
echo Building simple_paint with detailed output...
cmake --build . --target simple_paint --config Debug --verbose > paint_build.log 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo simple_paint build failed. See paint_build.log for details.
    notepad paint_build.log
) else (
    echo simple_paint built successfully.
)

REM Check for vcpkg integration
set VCPKG_ROOT=
for %%p in (
    "C:\vcpkg"
    "C:\dev\vcpkg"
    "%USERPROFILE%\vcpkg"
) do (
    if exist "%%p\vcpkg.exe" (
        set VCPKG_ROOT=%%p
        goto :found_vcpkg
    )
)

:found_vcpkg
if "!VCPKG_ROOT!" == "" (
    echo VCPKG not found in common locations.
    echo To build model_viewer, you need to install VCPKG first.
    echo See setup_3d_deps.bat for instructions.
) else (
    echo Found vcpkg at: !VCPKG_ROOT!
    echo Building model_viewer...
    
    REM Reconfigure with vcpkg toolchain
    cmake -DCMAKE_TOOLCHAIN_FILE=!VCPKG_ROOT!\scripts\buildsystems\vcpkg.cmake .. 2> vcpkg_errors.txt
    
    if %ERRORLEVEL% NEQ 0 (
        echo CMake reconfiguration with vcpkg failed. See vcpkg_errors.txt for details.
        notepad vcpkg_errors.txt
    ) else (
        REM Build the model_viewer application
        cmake --build . --target model_viewer --config Debug --verbose > model_build.log 2>&1
        
        if %ERRORLEVEL% NEQ 0 (
            echo model_viewer build failed. See model_build.log for details.
            notepad model_build.log
        ) else (
            echo model_viewer built successfully.
        )
    )
)

echo.
echo Build process complete. The following apps were processed:
echo  - simple_paint: Check bin\Debug\simple_paint.exe
echo  - model_viewer: Check bin\Debug\model_viewer.exe (if dependencies were found)
echo.
echo Log files are available in the build directory:
echo  - paint_build.log: simple_paint build output
echo  - model_build.log: model_viewer build output (if built)
echo.

cd ..
pause 