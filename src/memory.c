#include "../include/memory.h"
#include "../include/video.h"  // Include video.h for video_update_palette
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

// I/O ports and hardware registers
static uint8_t io_ports[256];
static uint8_t interrupt_enable = 0;
static uint8_t sound_enable = 0;
static uint8_t flip_screen = 0;
static uint8_t lamp1 = 0;
static uint8_t lamp2 = 0;
static uint8_t coin_lockout = 0;
static uint8_t coin_counter = 0;
static uint8_t watchdog_counter = 0;

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
    if (!filepath) return false;
    
    FILE *file = fopen(filepath, "rb");
    if (file) {
        fclose(file);
        printf("File exists: %s\n", filepath);
        return true;
    }
    printf("File NOT found: %s\n", filepath);
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
    
    // Check if allocation succeeded
    if (!rom || !ram || !vram || !cram || !charset || !sprites || !palette) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        memory_cleanup();
        return false;
    }
    
    // Initialize memory to prevent uninitialized access
    memset(rom, 0, ROM_SIZE);
    printf("Memory allocation successful, initializing...\n");
    
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
    
    printf("Loading ROMs from directory: %s\n", rom_dir);
    
    // Load program ROMs
    char *path;
    bool success = true;
    
    // Pacman program ROM 1
    path = build_path(rom_dir, pacman_roms.program1);
    printf("Checking for ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading ROM: %s\n", path);
        success &= load_rom_file(path, rom, ROM_PACMAN1);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 2
    path = build_path(rom_dir, pacman_roms.program2);
    printf("Checking for ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading ROM: %s\n", path);
        success &= load_rom_file(path, rom + ROM_PACMAN1, ROM_PACMAN2);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 3
    path = build_path(rom_dir, pacman_roms.program3);
    printf("Checking for ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading ROM: %s\n", path);
        success &= load_rom_file(path, rom + ROM_PACMAN1 + ROM_PACMAN2, ROM_PACMAN3);
    } else {
        fprintf(stderr, "Program ROM not found: %s\n", path);
        success = false;
    }
    free(path);
    
    // Pacman program ROM 4
    path = build_path(rom_dir, pacman_roms.program4);
    printf("Checking for ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading ROM: %s\n", path);
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
    printf("Checking for character ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading character ROM: %s\n", path);
        if (load_rom_file(path, temp_buffer, 0x1000)) {
            // Convert from MAME format to our format
            for (int i = 0; i < 256; i++) {
                for (int j = 0; j < 8; j++) {
                    charset[i * 8 + j] = temp_buffer[i * 16 + j];
                }
            }
            printf("Character ROM loaded successfully\n");
        } else {
            success = false;
            printf("Failed to load character ROM\n");
        }
    } else {
        fprintf(stderr, "Graphics ROM not found: %s\n", path);
        // Not fatal, we'll use placeholder graphics
        printf("Using placeholder character graphics\n");
    }
    free(path);
    
    // Sprite ROM (gfx2)
    path = build_path(rom_dir, pacman_roms.gfx2);
    printf("Checking for sprite ROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading sprite ROM: %s\n", path);
        if (load_rom_file(path, temp_buffer, 0x1000)) {
            // Convert from MAME format to our format
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 16; j++) {
                    sprites[i * 16 + j] = temp_buffer[i * 16 + j];
                }
            }
            printf("Sprite ROM loaded successfully\n");
        } else {
            success = false;
            printf("Failed to load sprite ROM\n");
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
    printf("Checking for palette PROM: %s\n", path);
    if (file_exists(path)) {
        printf("Loading palette PROM: %s\n", path);
        if (load_rom_file(path, palette_prom, 32)) {
            // Convert palette PROM to RGBA
            for (int i = 0; i < 32; i++) {
                uint8_t c = palette_prom[i];
                uint8_t r = (c & 0x07) * 36;  // 3 bits of red
                uint8_t g = ((c >> 3) & 0x07) * 36; // 3 bits of green
                uint8_t b = ((c >> 6) & 0x03) * 85; // 2 bits of blue
                palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
            printf("Palette PROM loaded successfully\n");
        } else {
            // Not fatal, we'll use default palette
            printf("Using default color palette (load failed)\n");
        }
    } else {
        fprintf(stderr, "Palette PROM not found: %s\n", path);
        // Not fatal, we'll use default palette
        printf("Using default color palette (file not found)\n");
    }
    free(path);
    
    // Print final status
    printf("ROM loading %s\n", success ? "succeeded" : "failed");
    
    // Initialize palette with some basic colors regardless
    printf("Ensuring default colors are set\n");
    for (int i = 0; i < 32; i++) {
        // Make sure we have some visible colors (might be overwritten by ROM loading)
        uint8_t r = ((i & 1) ? 0xFF : 0);
        uint8_t g = ((i & 2) ? 0xFF : 0);
        uint8_t b = ((i & 4) ? 0xFF : 0);
        
        // Set the palette directly
        if (palette) {
            palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    
    // Always call memory_reset to set up default sprite positions and other state
    printf("Initializing memory defaults\n");
    memory_reset();
    
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
    
    // Initialize some values for testing
    interrupt_enable = 1;  // Enable interrupts by default for testing
    sound_enable = 1;      // Enable sound
    flip_screen = 0;       // No screen flip
    
    // Set some default sprite values near the end of VRAM
    if (vram && VRAM_SIZE >= 0x1000) {
        // Set sprite 0 (Pacman)
        vram[0xFF0] = 0 << 2;  // Sprite 0, no flip
        vram[0xFF1] = 6;       // Yellow color
        
        // Set sprite 1 (Ghost 1)
        vram[0xFF2] = 1 << 2;  // Sprite 1, no flip
        vram[0xFF3] = 4;       // Red color
        
        // Set sprite 2 (Ghost 2)
        vram[0xFF4] = 2 << 2;  // Sprite 2, no flip
        vram[0xFF5] = 1;       // Blue color
        
        // Set sprite 3 (Ghost 3)
        vram[0xFF6] = 3 << 2;  // Sprite 3, no flip
        vram[0xFF7] = 2;       // Green color
    }
    
    // Set default sprite positions in I/O ports
    io_ports[0x60] = 100;      // Sprite 0 X
    io_ports[0x61] = 100;      // Sprite 0 Y
    io_ports[0x62] = 150;      // Sprite 1 X
    io_ports[0x63] = 100;      // Sprite 1 Y
    io_ports[0x64] = 100;      // Sprite 2 X
    io_ports[0x65] = 150;      // Sprite 2 Y
    io_ports[0x66] = 150;      // Sprite 3 X
    io_ports[0x67] = 150;      // Sprite 3 Y
}

// Read a byte from memory (based on MAME pacman_map implementation)
uint8_t memory_read_byte(uint16_t address) {
    // Memory map (based on MAME documentation):
    // 0000-3FFF: ROM (16KB)
    // 4000-43FF: Video RAM (1KB)
    // 4400-47FF: Color RAM (1KB)
    // 4800-4FFF: Work RAM (2KB)
    // 5000-5FFF: I/O Ports
    
    if (address <= ROM_END) {
        // ROM (0x0000-0x3FFF)
        return rom[address];
    } else if (address >= VRAM_START && address <= VRAM_END) {
        // Video RAM (0x4000-0x43FF)
        return vram[address - VRAM_START];
    } else if (address >= CRAM_START && address <= CRAM_END) {
        // Color RAM (0x4400-0x47FF)
        return cram[address - CRAM_START];
    } else if (address >= WRAM_START && address <= WRAM_END) {
        // Work RAM (0x4800-0x4FFF)
        return ram[address - WRAM_START];
    } else if (address == IO_IN0) {
        // Input port 0 (0x5000) - Player 1 controls, coin, service
        return io_read_byte(address & 0xFF);
    } else if (address == IO_IN1) {
        // Input port 1 (0x5040) - Player 2 controls
        return io_read_byte(address & 0xFF);
    } else if (address == IO_DSW1) {
        // DIP switches 1 (0x5080) - Game settings
        return io_read_byte(address & 0xFF);
    } else if (address == IO_DSW2) {
        // DIP switches 2 (0x50C0) - Only on some games
        return io_read_byte(address & 0xFF);
    } else if (address >= 0x5000 && address <= 0x50FF) {
        // Other I/O ports (I/O range is 0x5000-0x50FF)
        return io_read_byte(address & 0xFF);
    }
    
    // Default for unmapped addresses
    // printf("Warning: Read from unmapped address: 0x%04X\n", address);
    return 0xFF;
}

// Write a byte to memory (based on MAME pacman_map implementation)
void memory_write_byte(uint16_t address, uint8_t value) {
    if (address <= ROM_END) {
        // ROM is read-only, ignore writes
        return;
    } else if (address >= VRAM_START && address <= VRAM_END) {
        // Video RAM is writable (0x4000-0x43FF)
        vram[address - VRAM_START] = value;
    } else if (address >= CRAM_START && address <= CRAM_END) {
        // Color RAM is writable (0x4400-0x47FF)
        cram[address - CRAM_START] = value;
    } else if (address >= WRAM_START && address <= WRAM_END) {
        // Work RAM is writable (0x4800-0x4FFF)
        ram[address - WRAM_START] = value;
        
        // Special case: sprite data (0x4FF0-0x4FFF)
        // In Pacman, 8 pairs of bytes at 0x4FF0-0x4FFF define sprite attributes
        // First byte: sprite image number (bits 2-7), Y flip (bit 0), X flip (bit 1)
        // Second byte: color
    } else if (address == INTERRUPT_EN) {
        // 0x5000: Interrupt enable
        interrupt_enable = value & 0x01;
    } else if (address == SOUND_EN) {
        // 0x5001: Sound enable
        sound_enable = value & 0x01;
    } else if (address == FLIP_SCREEN) {
        // 0x5003: Flip screen
        flip_screen = value & 0x01;
    } else if (address == PLAYER1_LAMP) {
        // 0x5004: 1 player start lamp
        lamp1 = value & 0x01;
    } else if (address == PLAYER2_LAMP) {
        // 0x5005: 2 players start lamp
        lamp2 = value & 0x01;
    } else if (address == COIN_LOCKOUT) {
        // 0x5006: Coin lockout
        coin_lockout = value & 0x01;
    } else if (address == COIN_COUNTER) {
        // 0x5007: Coin counter
        coin_counter = value & 0x01;
    } else if (address >= SOUND_REG_START && address <= SOUND_REG_END) {
        // 0x5040-0x505F: Sound registers
        // These registers control the Namco sound hardware
        io_write_byte((address & 0xFF), value);
    } else if (address >= SPRITE_COORD && address <= SPRITE_COORD + 0x0F) {
        // 0x5060-0x506F: Sprite coordinates, x/y pairs for 8 sprites
        io_write_byte((address & 0xFF), value);
    } else if (address == WATCHDOG) {
        // 0x50C0: Watchdog reset
        watchdog_counter = 0;
    } else if (address >= 0x5000 && address <= 0x50FF) {
        // Other I/O ports (I/O range is 0x5000-0x50FF)
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

// I/O port read (based on MAME implementation)
uint8_t io_read_byte(uint8_t port) {
    // According to MAME, Pacman has the following I/O reads:
    // 0x5000 - IN0 - Player 1 controls, coin, service
    // 0x5040 - IN1 - Player 2 controls
    // 0x5080 - DSW1 - Game settings
    // 0x50C0 - DSW2 - Only on some games (Ponpoko, mschamp, Van Van Car, Rock and Roll Trivia 2)
    
    switch (port) {
        case (IO_IN0 & 0xFF): // IN0 - Player 1 controls, coin, service (0x00)
            // Bit 0: UP
            // Bit 1: LEFT
            // Bit 2: RIGHT
            // Bit 3: DOWN
            // Bit 4: Special control (rack advance, button, etc. depending on game)
            // Bit 5: Start 1
            // Bit 6: Start 2
            // Bit 7: Coin
            return io_ports[port];
            
        case (IO_IN1 & 0xFF): // IN1 - Player 2 controls (0x40)
            // Bit 0: UP
            // Bit 1: LEFT
            // Bit 2: RIGHT
            // Bit 3: DOWN
            // Bit 4: Special control
            // Bit 5: Not used
            // Bit 6: Not used
            // Bit 7: Not used
            return io_ports[port];
            
        case (IO_DSW1 & 0xFF): // DSW1 - Dipswitch settings (0x80)
            // Bits 0-1: Lives (00 = 1 life, 01 = 2 lives, 10 = 3 lives, 11 = 5 lives)
            // Bit 2: Cabinet type (0 = upright, 1 = cocktail)
            // Bits 3-4: Difficulty (00 = hardest, 11 = easiest)
            // Bits 5-6: Bonus (00 = 10000, 01 = 15000, 10 = 20000, 11 = no bonus)
            // Bit 7: Coin (0 = 1 coin/1 game, 1 = 1 coin/2 games)
            return io_ports[port];
            
        case (IO_DSW2 & 0xFF): // DSW2 - Dipswitches on some games (0xC0)
            // Game-specific switches
            return io_ports[port];
            
        // Note: Watchdog and DSW2 share the same address (0x50C0)
        // We handle this by priority - DSW2 for reads, Watchdog for writes
            
        default:
            return io_ports[port];
    }
}

// I/O port write (based on MAME implementation)
void io_write_byte(uint8_t port, uint8_t value) {
    // According to MAME, Pacman has the following I/O writes:
    // 0x5000 - Interrupt enable
    // 0x5001 - Sound enable
    // 0x5002 - No connection (not used)
    // 0x5003 - Flip screen
    // 0x5004 - Player 1 start lamp
    // 0x5005 - Player 2 start lamp
    // 0x5006 - Coin lockout (not implemented on most boards)
    // 0x5007 - Coin counter
    // 0x5040-0x505F - Sound registers
    // 0x5060-0x506F - Sprite coordinates
    // 0x50C0 - Watchdog reset
    
    switch (port) {
        case (INTERRUPT_EN & 0xFF): // Interrupt enable (0x00)
            interrupt_enable = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (SOUND_EN & 0xFF): // Sound enable (0x01)
            sound_enable = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (FLIP_SCREEN & 0xFF): // Flip screen (0x03)
            flip_screen = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (PLAYER1_LAMP & 0xFF): // Player 1 start lamp (0x04)
            lamp1 = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (PLAYER2_LAMP & 0xFF): // Player 2 start lamp (0x05)
            lamp2 = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (COIN_LOCKOUT & 0xFF): // Coin lockout (0x06)
            coin_lockout = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (COIN_COUNTER & 0xFF): // Coin counter (0x07)
            coin_counter = value & 0x01;
            io_ports[port] = value;
            break;
            
        case (WATCHDOG & 0xFF): // Watchdog reset (0xC0)
            watchdog_counter = 0;
            io_ports[port] = value;
            break;
            
        default:
            // Handle sound registers (0x40-0x5F)
            if (port >= (SOUND_REG_START & 0xFF) && port <= (SOUND_REG_END & 0xFF)) {
                // Sound voice registers - to implement Namco sound
            }
            // Handle sprite coordinates (0x60-0x6F)
            else if (port >= (SPRITE_COORD & 0xFF) && port <= ((SPRITE_COORD & 0xFF) + 0x0F)) {
                // Sprite coordinates
            }
            
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

// Getter functions for hardware flags
uint8_t memory_get_interrupt_enable(void) {
    return interrupt_enable;
}

uint8_t memory_get_sound_enable(void) {
    return sound_enable;
}

uint8_t memory_get_flip_screen(void) {
    return flip_screen;
}

uint8_t memory_get_lamp1(void) {
    return lamp1;
}

uint8_t memory_get_lamp2(void) {
    return lamp2;
}

uint8_t memory_get_coin_lockout(void) {
    return coin_lockout;
}

uint8_t memory_get_coin_counter(void) {
    return coin_counter;
}

// Set input ports - for external input handling
void memory_set_input_port(uint8_t port, uint8_t value) {
    io_ports[port] = value;
}