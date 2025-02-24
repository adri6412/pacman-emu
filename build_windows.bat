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

REM Check if the directory exists - check all possible SDL2 include structures
if exist "C:\SDL2-2.0.14\include\SDL2\SDL.h" (
    set SDL_INCLUDE_PATH=C:\SDL2-2.0.14\include
    echo Found SDL2 headers in C:\SDL2-2.0.14\include\SDL2\SDL.h
) else if exist "C:\SDL2-2.0.14\include\SDL.h" (
    set SDL_INCLUDE_PATH=C:\SDL2-2.0.14\include
    echo Found SDL2 headers in C:\SDL2-2.0.14\include\SDL.h
) else if exist "C:\SDL2-2.0.14\SDL2\include\SDL.h" (
    set SDL_INCLUDE_PATH=C:\SDL2-2.0.14\SDL2\include
    echo Found SDL2 headers in C:\SDL2-2.0.14\SDL2\include\SDL.h
) else if exist "C:\SDL2-2.0.14\x86_64-w64-mingw32\include\SDL2\SDL.h" (
    set SDL_INCLUDE_PATH=C:\SDL2-2.0.14\x86_64-w64-mingw32\include
    echo Found SDL2 headers in C:\SDL2-2.0.14\x86_64-w64-mingw32\include\SDL2\SDL.h
) else (
    echo Could not find SDL2 headers in any standard location.
    echo Checking C:\SDL2-2.0.14 directory structure...
    dir "C:\SDL2-2.0.14" /s /b | findstr SDL.h
    echo.
    echo Please check your SDL2 installation.
    exit /b 1
)

REM Run make with explicit paths for your system
echo Building with MinGW...

REM Set path to your MinGW installation
set PATH=G:\msys64\mingw64\bin;G:\msys64\usr\bin;%PATH%

REM Set explicit make command
set MAKE_CMD=G:\msys64\mingw64\bin\mingw32-make.exe
if not exist "%MAKE_CMD%" set MAKE_CMD=G:\msys64\usr\bin\make.exe

echo Executing: %MAKE_CMD%
%MAKE_CMD% SDL2_DIR=C:\SDL2-2.0.14 || (
    echo Build failed.
    exit /b 1
)

REM Copy SDL2.dll if it exists in various possible locations
if not exist "bin" mkdir bin

echo Looking for SDL2.dll...
if exist "C:\SDL2-2.0.14\bin\SDL2.dll" (
    echo Copying SDL2.dll from C:\SDL2-2.0.14\bin to bin directory...
    copy "C:\SDL2-2.0.14\bin\SDL2.dll" bin\
) else if exist "C:\SDL2-2.0.14\lib\x64\SDL2.dll" (
    echo Copying SDL2.dll from C:\SDL2-2.0.14\lib\x64 to bin directory...
    copy "C:\SDL2-2.0.14\lib\x64\SDL2.dll" bin\
) else if exist "C:\SDL2-2.0.14\x86_64-w64-mingw32\bin\SDL2.dll" (
    echo Copying SDL2.dll from C:\SDL2-2.0.14\x86_64-w64-mingw32\bin to bin directory...
    copy "C:\SDL2-2.0.14\x86_64-w64-mingw32\bin\SDL2.dll" bin\
) else (
    echo.
    echo WARNING: SDL2.dll not found in any standard location.
    echo Searching for SDL2.dll in C:\SDL2-2.0.14...
    dir "C:\SDL2-2.0.14" /s /b | findstr SDL2.dll
    echo.
    echo You'll need to copy SDL2.dll manually to the bin directory.
)

echo.
echo Build completed successfully! 
echo The emulator is located in the bin directory.
echo To run: bin\pacman-emu.exe data\test.rom

endlocal