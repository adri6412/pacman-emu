# Pacman Emulator

A simple Pacman arcade emulator built in C using SDL2.

## Features

- Z80 CPU emulation (simplified)
- Pacman hardware emulation
- SDL2-based graphics output
- Keyboard input handling
- Support for original Pacman ROM

## Requirements

- C compiler (gcc or clang)
- SDL2 library
- Pacman ROM file (not included)

## Building

### Linux

Make sure you have SDL2 installed on your system. On Ubuntu/Debian, you can install it with:

```
sudo apt-get install libsdl2-dev
```

Then compile the emulator:

```
make
```

This will create the executable in the `bin` directory.

### Windows (MinGW64)

1. Install MinGW-w64 with MSYS2 (https://www.msys2.org/)
2. Install the required packages:
   ```
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-SDL2
   ```
3. Download SDL2 development libraries for MinGW from https://www.libsdl.org/download-2.0.php
4. Extract SDL2 to a directory, e.g., `C:\SDL2-2.0.14`
5. Set the SDL2_DIR environment variable:
   ```
   set SDL2_DIR=C:\SDL2-2.0.14
   ```
6. Run the build script:
   ```
   build_windows.bat
   ```
   
Alternatively, you can build manually:
   ```
   mingw32-make
   copy %SDL2_DIR%\bin\SDL2.dll bin\
   ```

The executable will be created in the `bin` directory with the name `pacman-emu.exe`.

## Usage

### Command Line Options

```
pacman-emu [options] [rom_path]
```

Options:
- `--help` - Show help message
- `--test` - Use built-in test ROM (no external ROM needed)

### Testing Without a ROM

You can run the emulator with the built-in test ROM:

```
./bin/pacman-emu --test
```

This will use the included test ROM that draws a simple pattern on the screen.

### Using a Single ROM File

```
./bin/pacman-emu /path/to/pacman.rom
```

### Using a MAME ROM Set

The emulator can use original MAME Pacman ROM sets:

```
./bin/pacman-emu /path/to/mame/pacman/roms
```

The emulator will look for the following files in the directory:

- `pacman.6e` - Program ROM 1
- `pacman.6f` - Program ROM 2
- `pacman.6h` - Program ROM 3
- `pacman.6j` - Program ROM 4
- `pacman.5e` - Character ROM
- `pacman.5f` - Sprite ROM
- `82s123.7f` - Color palette PROM

These files are included in the standard MAME Pacman ROM set.

## Controls

- Arrow keys: Move Pacman (Player 1)
- W, A, S, D: Move (Player 2)
- 5: Insert coin
- 1: Player 1 start
- 2: Player 2 start
- F1: Service button
- ESC: Quit

## Implementation Notes

This is a simplified emulator for educational purposes. It doesn't implement the full Z80 instruction set or all the hardware features of the original Pacman arcade machine. The main components are:

- **CPU**: Z80 processor emulation (partial implementation)
- **Memory**: ROM, RAM, Video RAM, and Color RAM
- **Video**: Tile-based background and sprite rendering
- **Input**: Keyboard mapping to Pacman controls

## License

This project is released under the MIT License.

## Acknowledgements

This emulator is inspired by the original Pacman arcade hardware and various open-source emulation projects.

## Disclaimer

This project is for educational purposes only. You must own the original Pacman ROM to use this emulator legally.