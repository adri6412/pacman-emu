#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stdint.h>

// Pacman memory map constants
#define ROM_SIZE        0x4000  // 16KB ROM (multiple chips)
#define RAM_SIZE        0x1000  // 4KB RAM
#define VRAM_SIZE       0x0400  // 1KB Video RAM
#define CRAM_SIZE       0x0400  // 1KB Color RAM
#define PROM_SIZE       0x0120  // Color palette + sprite lookup PROMs

// Main ROM components for Pacman (MAME format)
#define ROM_PACMAN1     0x1000  // pacman.6e
#define ROM_PACMAN2     0x1000  // pacman.6f
#define ROM_PACMAN3     0x1000  // pacman.6h
#define ROM_PACMAN4     0x1000  // pacman.6j

// Memory map addresses
#define ROM_START       0x0000
#define ROM_END         0x3FFF
#define RAM_START       0x4000
#define RAM_END         0x4FFF
#define VRAM_START      0x5000
#define VRAM_END        0x53FF
#define CRAM_START      0x5400
#define CRAM_END        0x57FF
#define IO_START        0x5000
#define IO_END          0x5FFF

// MAME ROM filenames
typedef struct {
    const char *program1;     // pacman.6e
    const char *program2;     // pacman.6f
    const char *program3;     // pacman.6h
    const char *program4;     // pacman.6j
    const char *gfx1;         // pacman.5e - Tile data
    const char *gfx2;         // pacman.5f - Sprite data
    const char *palette;      // 82s123.7f - Color palette
    const char *colortable;   // 82s126.4a - Color lookup
    const char *charmap;      // 82s126.1m - Character lookup
} MameRomSet;

// Function prototypes
bool memory_init(const char *rom_path);
bool memory_init_mame_set(const char *rom_dir);
void memory_cleanup(void);
void memory_reset(void);

uint8_t memory_read_byte(uint16_t address);
void memory_write_byte(uint16_t address, uint8_t value);

uint16_t memory_read_word(uint16_t address);
void memory_write_word(uint16_t address, uint16_t value);

// I/O port functions
uint8_t io_read_byte(uint8_t port);
void io_write_byte(uint8_t port, uint8_t value);

// Memory access for other components
uint8_t* memory_get_vram(void);
uint8_t* memory_get_cram(void);
uint8_t* memory_get_charset(void);
uint8_t* memory_get_spritedata(void);
uint32_t* memory_get_palette(void);

#endif // MEMORY_H