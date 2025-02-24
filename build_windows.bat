@echo off
setlocal enabledelayedexpansion

echo Pacman Emulator Windows Build Script

REM Check if SDL2_DIR is set
if "%SDL2_DIR%"=="" (
    echo SDL2_DIR environment variable is not set.
    echo Please set it to your SDL2 installation directory.
    echo Example:
    echo    set SDL2_DIR=C:\SDL2-2.0.14
    exit /b 1
)

echo Using SDL2 from: %SDL2_DIR%

REM Check if the directory exists
if not exist "%SDL2_DIR%\include\SDL2\SDL.h" (
    echo Could not find SDL2 headers in %SDL2_DIR%\include\SDL2\SDL.h
    echo Please check your SDL2_DIR setting.
    exit /b 1
)

REM Run make
echo Building with MinGW...
mingw32-make || (
    echo Build failed.
    exit /b 1
)

REM Copy SDL2.dll if it exists
if exist "%SDL2_DIR%\bin\SDL2.dll" (
    echo Copying SDL2.dll to bin directory...
    copy "%SDL2_DIR%\bin\SDL2.dll" bin\
) else (
    echo.
    echo WARNING: SDL2.dll not found in %SDL2_DIR%\bin
    echo You'll need to ensure SDL2.dll is in your PATH or copy it manually
    echo to the bin directory before running the emulator.
)

echo.
echo Build completed successfully! 
echo The emulator is located in the bin directory.
echo To run: bin\pacman-emu.exe data\test.rom

endlocal