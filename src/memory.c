#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
    #include <direct.h>
    #define PATH_SEPARATOR '\\'
    #define MKDIR(dir, mode) _mkdir(dir)
#else
    #include <unistd.h>
    #define PATH_SEPARATOR '/'
    #define MKDIR(dir, mode) mkdir(dir, mode)
#endif

// Memory segments
static uint8_t *rom = NULL;
static uint8_t *ram = NULL;
static uint8_t *vram = NULL;
static uint8_t *cram = NULL;
static uint8_t *charset = NULL;  // Character ROM
static uint8_t *sprites = NULL;  // Sprite ROM
static uint32_t *palette = NULL; // Color palette

// I/O ports
static uint8_t io_ports[256];

// Standard Pacman MAME ROM filenames
static const MameRomSet pacman_roms = {
    .program1 = "pacman.6e",
    .program2 = "pacman.6f",
    .program3 = "pacman.6h",
    .program4 = "pacman.6j",
    .gfx1 = "pacman.5e",
    .gfx2 = "pacman.5f",
    .palette = "82s123.7f",
    .colortable = "82s126.4a",
    .charmap = "82s126.1m"
};

// Load a single ROM file
static bool load_rom_file(const char *filepath, uint8_t *buffer, size_t size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open ROM file: %s\n", filepath);
        return false;
    }
    
    size_t read_size = fread(buffer, 1, size, file);
    fclose(file);
    
    if (read_size == 0) {
        fprintf(stderr, "Failed to read ROM file: %s\n", filepath);
        return false;
    }
    
    if (read_size < size) {
        fprintf(stderr, "ROM file size mismatch: %s - expected %zu bytes, read %zu bytes\n", 
                filepath, size, read_size);
        // Pad with 0xFF (RST 38h opcode)
        memset(buffer + read_size, 0xFF, size - read_size);
        printf("ROM padded to full size with 0xFF bytes\n");
    }
    
    return true;
}

// Check if a file exists
static bool file_exists(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

// Build a full path
static char* build_path(const char *dir, const char *file) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    char *path = malloc(dir_len + file_len + 2); // +2 for separator and null terminator
    if (!path) return NULL;
    
    strcpy(path, dir);
    
    // Add path separator if needed
    if (dir[dir_len - 1] != PATH_SEPARATOR) {
        path[dir_len] = PATH_SEPARATOR;
        strcpy(path + dir_len + 1, file);
    } else {
        strcpy(path + dir_len, file);
    }
    
    return path;
}

// Initialize memory with a single ROM file (legacy support)
bool memory_init(const char *rom_path) {
    // Allocate memory
    rom = (uint8_t *)malloc(ROM_SIZE);
    ram = (uint8_t *)malloc(RAM_SIZE);
    vram = (uint8_t *)malloc(VRAM_SIZE);
    cram = (uint8_t *)malloc(CRAM_SIZE);
    charset = (uint8_t *)malloc(256 * 8); // 256 characters, 8 bytes per character
    sprites = (uint8_t *)malloc(64 * 16); // 64 sprites, 16 bytes per sprite
    palette = (uint32_t *)malloc(256 * sizeof(uint32_t)); // 256 colors
    
    // Initialize memory to prevent uninitialized access
    if (rom) memset(rom, 0, ROM_SIZE);
    
    if (!rom || !ram || !vram || !cram || !charset || !sprites || !palette) {
        fprintf(stderr, "Failed to allocate memory\n");
        memory_cleanup();
        return false;
    }
    
    // Clear memory
    memset(ram, 0, RAM_SIZE);
    memset(vram, 0, VRAM_SIZE);
    memset(cram, 0, CRAM_SIZE);
    memset(io_ports, 0, sizeof(io_ports));
    
    // Initialize character set with better patterns for our test ROM
    // First character (0) is space - all zeros
    memset(&charset[0], 0, 8);
    
    // Character 1 (H) - Simple H pattern
    charset[1*8+0] = 0xC3; // 11000011
    charset[1*8+1] = 0xC3; // 11000011
    charset[1*8+2] = 0xC3; // 11000011
    charset[1*8+3] = 0xFF; // 11111111
    charset[1*8+4] = 0xFF; // 11111111
    charset[1*8+5] = 0xC3; // 11000011
    charset[1*8+6] = 0xC3; // 11000011
    charset[1*8+7] = 0xC3; // 11000011
    
    // Character 2 (E) - Simple E pattern
    charset[2*8+0] = 0xFF; // 11111111
    charset[2*8+1] = 0xFF; // 11111111
    charset[2*8+2] = 0xC0; // 11000000
    charset[2*8+3] = 0xFF; // 11111111
    charset[2*8+4] = 0xFF; // 11111111
    charset[2*8+5] = 0xC0; // 11000000
    charset[2*8+6] = 0xFF; // 11111111
    charset[2*8+7] = 0xFF; // 11111111
    
    // Character 3 (L) - Simple L pattern
    charset[3*8+0] = 0xC0; // 11000000
    charset[3*8+1] = 0xC0; // 11000000
    charset[3*8+2] = 0xC0; // 11000000
    charset[3*8+3] = 0xC0; // 11000000
    charset[3*8+4] = 0xC0; // 11000000
    charset[3*8+5] = 0xC0; // 11000000
    charset[3*8+6] = 0xFF; // 11111111
    charset[3*8+7] = 0xFF; // 11111111
    
    // Character 4 (O) - Simple O pattern
    charset[4*8+0] = 0x7E; // 01111110
    charset[4*8+1] = 0xFF; // 11111111
    charset[4*8+2] = 0xC3; // 11000011
    charset[4*8+3] = 0xC3; // 11000011
    charset[4*8+4] = 0xC3; // 11000011
    charset[4*8+5] = 0xC3; // 11000011
    charset[4*8+6] = 0xFF; // 11111111
    charset[4*8+7] = 0x7E; // 01111110
    
    // Character 5 (W) - Simple W pattern
    charset[5*8+0] = 0xC3; // 11000011
    charset[5*8+1] = 0xC3; // 11000011
    charset[5*8+2] = 0xC3; // 11000011
    charset[5*8+3] = 0xC3; // 11000011
    charset[5*8+4] = 0xDB; // 11011011
    charset[5*8+5] = 0xFF; // 11111111
    charset[5*8+6] = 0x66; // 01100110
    charset[5*8+7] = 0x24; // 00100100
    
    // Character 6 (R) - Simple R pattern
    charset[6*8+0] = 0xFC; // 11111100
    charset[6*8+1] = 0xFE; // 11111110
    charset[6*8+2] = 0xC3; // 11000011
    charset[6*8+3] = 0xFE; // 11111110
    charset[6*8+4] = 0xFC; // 11111100
    charset[6*8+5] = 0xDE; // 11011110
    charset[6*8+6] = 0xCF; // 11001111
    charset[6*8+7] = 0xC7; // 11000111
    
    // Character 7 (D) - Simple D pattern
    charset[7*8+0] = 0xFC; // 11111100
    charset[7*8+1] = 0xFE; // 11111110
    charset[7*8+2] = 0xC3; // 11000011
    charset[7*8+3] = 0xC3; // 11000011
    charset[7*8+4] = 0xC3; // 11000011
    charset[7*8+5] = 0xC3; // 11000011
    charset[7*8+6] = 0xFE; // 11111110
    charset[7*8+7] = 0xFC; // 11111100
    
    // Fill the rest with simple patterns
    for (int i = 8; i < 256; i++) {
        for (int j = 0; j < 8; j++) {
            charset[i * 8 + j] = ((i + j) % 8) ? 0x00 : 0xFF;
        }
    }
    
    // Clear sprite data
    memset(sprites, 0, 64 * 16);
    
    // Initialize palette with default colors
    for (int i = 0; i < 256; i++) {
        palette[i] = 0xFF000000 | // Alpha
                    ((i & 4) ? 0xFF0000 : 0) | // Red
                    ((i & 2) ? 0x00FF00 : 0) | // Green
                    ((i & 1) ? 0x0000FF : 0);  // Blue
    }
    
    // Load ROM file - first check if it's a directory or a single file
    struct stat path_stat;
    stat(rom_path, &path_stat);
    
    if (S_ISDIR(path_stat.st_mode)) {
        // It's a directory, try to load MAME ROM set
        return memory_init_mame_set(rom_path);
    }
    
    // It's a file, try to load as a single ROM
    if (!load_rom_file(rom_path, rom, ROM_SIZE)) {
        fprintf(stderr, "Failed to load ROM file\n");
        return false;
    }
    
    return true;
}

// Initialize memory with MAME ROM set
bool memory_init_mame_set(const char *rom_dir) {
    if (!rom || !ram || !vram || !cram || !charset || !sprites || !palette) {
        // Memory not allocated yet, initialize it
        if (!memory_init("/dev/null")) {
            return false;
        }
    }
    
    // Load program ROMs
    char *path;
    bool success = true;
    
    // Pacman program ROM 1
    path = build_path(rom_dir, pacman_roms.program1);
    if (file_exists(path)) {
        success &= load_rom_file(path, rom, ROM_PACMAN1);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 2
    path = build_path(rom_dir, pacman_roms.program2);
    if (file_exists(path)) {
        success &= load_rom_file(path, rom + ROM_PACMAN1, ROM_PACMAN2);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 3
    path = build_path(rom_dir, pacman_roms.program3);
    if (file_exists(path)) {
        success &= load_rom_file(path, rom + ROM_PACMAN1 + ROM_PACMAN2, ROM_PACMAN3);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 4
    path = build_path(rom_dir, pacman_roms.program4);
    if (file_exists(path)) {
        success &= load_rom_file(path, rom + ROM_PACMAN1 + ROM_PACMAN2 + ROM_PACMAN3, ROM_PACMAN4);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Load graphics ROMs
    uint8_t *temp_buffer = (uint8_t *)malloc(0x1000); // Temp buffer for gfx ROMs
    if (!temp_buffer) {
        fprintf(stderr, "Failed to allocate temporary buffer\n");
        return false;
    }
    
    // Character ROM (gfx1)
    path = build_path(rom_dir, pacman_roms.gfx1);
    if (file_exists(path)) {
        if (load_rom_file(path, temp_buffer, 0x1000)) {
            // Convert from MAME format to our format
            for (int i = 0; i < 256; i++) {
                for (int j = 0; j < 8; j++) {
                    charset[i * 8 + j] = temp_buffer[i * 16 + j];
                }
            }
        } else {
            success = false;
        }
    } else {
        fprintf(stderr, "Graphics ROM not found: %s\n", path);
        // Not fatal, we'll use placeholder graphics
        printf("Using placeholder character graphics\n");
    }
    free(path);
    
    // Sprite ROM (gfx2)
    path = build_path(rom_dir, pacman_roms.gfx2);
    if (file_exists(path)) {
        if (load_rom_file(path, temp_buffer, 0x1000)) {
            // Convert from MAME format to our format
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 16; j++) {
                    sprites[i * 16 + j] = temp_buffer[i * 16 + j];
                }
            }
        } else {
            success = false;
        }
    } else {
        fprintf(stderr, "Graphics ROM not found: %s\n", path);
        // Not fatal, we'll use placeholder graphics
        printf("Using placeholder sprite graphics\n");
    }
    free(path);
    
    free(temp_buffer);
    
    // Load palette PROM
    uint8_t palette_prom[32];
    path = build_path(rom_dir, pacman_roms.palette);
    if (file_exists(path)) {
        if (load_rom_file(path, palette_prom, 32)) {
            // Convert palette PROM to RGBA
            for (int i = 0; i < 32; i++) {
                uint8_t c = palette_prom[i];
                uint8_t r = (c & 0x07) * 36;  // 3 bits of red
                uint8_t g = ((c >> 3) & 0x07) * 36; // 3 bits of green
                uint8_t b = ((c >> 6) & 0x03) * 85; // 2 bits of blue
                palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        } else {
            // Not fatal, we'll use default palette
            printf("Using default color palette\n");
        }
    } else {
        fprintf(stderr, "Palette PROM not found: %s\n", path);
        // Not fatal, we'll use default palette
        printf("Using default color palette\n");
    }
    free(path);
    
    return success;
}

// Clean up memory
void memory_cleanup(void) {
    if (rom) free(rom);
    if (ram) free(ram);
    if (vram) free(vram);
    if (cram) free(cram);
    if (charset) free(charset);
    if (sprites) free(sprites);
    if (palette) free(palette);
    
    rom = NULL;
    ram = NULL;
    vram = NULL;
    cram = NULL;
    charset = NULL;
    sprites = NULL;
    palette = NULL;
}

// Reset memory to initial state
void memory_reset(void) {
    if (ram) memset(ram, 0, RAM_SIZE);
    if (vram) memset(vram, 0, VRAM_SIZE);
    if (cram) memset(cram, 0, CRAM_SIZE);
    memset(io_ports, 0, sizeof(io_ports));
}

// Read a byte from memory
uint8_t memory_read_byte(uint16_t address) {
    // Memory map:
    // 0000-3FFF: ROM (16KB)
    // 4000-4FFF: RAM (4KB)
    // 5000-503F: Video RAM (1KB)
    // 5040-507F: Color RAM (1KB)
    // 5000-5FFF: I/O Ports (overlaps with VRAM and CRAM)
    
    if (address <= ROM_END) {
        return rom[address];
    } else if (address >= RAM_START && address <= RAM_END) {
        return ram[address - RAM_START];
    } else if (address >= VRAM_START && address <= VRAM_END) {
        return vram[address - VRAM_START];
    } else if (address >= CRAM_START && address <= CRAM_END) {
        return cram[address - CRAM_START];
    } else if (address >= IO_START && address <= IO_END) {
        // I/O ports - simplified for this implementation
        // In real Pacman, this would handle various hardware registers
        return io_read_byte((address & 0xFF));
    }
    
    // Default for unmapped addresses
    return 0xFF;
}

// Write a byte to memory
void memory_write_byte(uint16_t address, uint8_t value) {
    if (address <= ROM_END) {
        // ROM is read-only, ignore writes
        return;
    } else if (address >= RAM_START && address <= RAM_END) {
        ram[address - RAM_START] = value;
    } else if (address >= VRAM_START && address <= VRAM_END) {
        vram[address - VRAM_START] = value;
    } else if (address >= CRAM_START && address <= CRAM_END) {
        cram[address - CRAM_START] = value;
    } else if (address >= IO_START && address <= IO_END) {
        // I/O ports - simplified for this implementation
        io_write_byte((address & 0xFF), value);
    }
}

// Read a 16-bit word from memory
uint16_t memory_read_word(uint16_t address) {
    uint8_t low = memory_read_byte(address);
    uint8_t high = memory_read_byte(address + 1);
    return low | (high << 8);
}

// Write a 16-bit word to memory
void memory_write_word(uint16_t address, uint16_t value) {
    memory_write_byte(address, value & 0xFF);
    memory_write_byte(address + 1, value >> 8);
}

// I/O port read
uint8_t io_read_byte(uint8_t port) {
    // Map ports to specific hardware components in Pacman
    // Simplified implementation
    switch (port) {
        case 0x00: // IN0 - Player 1 controls
            return 0xFF; // Active low - no buttons pressed
            
        case 0x01: // IN1 - Player 2 controls
            return 0xFF; // Active low - no buttons pressed
            
        case 0x02: // DSW1 - Dipswitch settings
            return 0xFF; // All switches off
            
        default:
            return io_ports[port];
    }
}

// I/O port write
void io_write_byte(uint8_t port, uint8_t value) {
    // Map ports to specific hardware components in Pacman
    // Simplified implementation
    switch (port) {
        case 0x05: // Sound enable
            io_ports[port] = value;
            break;
            
        case 0x06: // Flip screen
            io_ports[port] = value;
            break;
            
        case 0x07: // Coin counter
            io_ports[port] = value;
            break;
            
        default:
            io_ports[port] = value;
            break;
    }
}

// Get access to VRAM for rendering
uint8_t* memory_get_vram(void) {
    return vram;
}

// Get access to color RAM for rendering
uint8_t* memory_get_cram(void) {
    return cram;
}

// Get access to character ROM
uint8_t* memory_get_charset(void) {
    return charset;
}

// Get access to sprite data
uint8_t* memory_get_spritedata(void) {
    return sprites;
}

// Get access to color palette
uint32_t* memory_get_palette(void) {
    return palette;
}