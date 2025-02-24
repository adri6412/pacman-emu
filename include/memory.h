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

// Memory map addresses (per MAME documentation)
#define ROM_START       0x0000  // 0000-3FFF ROM
#define ROM_END         0x3FFF
#define VRAM_START      0x4000  // 4000-43FF Video RAM
#define VRAM_END        0x43FF
#define CRAM_START      0x4400  // 4400-47FF Color RAM
#define CRAM_END        0x47FF
#define WRAM_START      0x4800  // 4800-4FFF Work RAM (Dream Shopper, Van Van Car use 4800-4BFF)
#define WRAM_END        0x4FFF
#define IO_START        0x5000  // IO port range start
#define IO_END          0x50FF  // IO port range end
#define IO_IN0          0x5000  // IN0 port
#define IO_IN1          0x5040  // IN1 port
#define IO_DSW1         0x5080  // DSW1 port
#define IO_DSW2         0x50C0  // DSW2 port (only on some games)
#define SPRITE_COORD    0x5060  // 5060-506F Sprite coordinates, x/y pairs for 8 sprites
#define WATCHDOG        0x50C0  // Watchdog reset
#define SOUND_REG_START 0x5040  // 5040-505F Sound voices registers
#define SOUND_REG_END   0x505F
#define SPRITES_START   0x4FF0  // 4FF0-4FFF 8 pairs of two bytes for sprites
#define SPRITES_END     0x4FFF
#define INTERRUPT_EN    0x5000  // Interrupt enable register
#define SOUND_EN        0x5001  // Sound enable register
#define FLIP_SCREEN     0x5003  // Flip screen
#define PLAYER1_LAMP    0x5004  // 1 player start lamp
#define PLAYER2_LAMP    0x5005  // 2 players start lamp
#define COIN_LOCKOUT    0x5006  // Coin lockout
#define COIN_COUNTER    0x5007  // Coin counter

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

// Getter functions for hardware flags and registers
uint8_t memory_get_interrupt_enable(void);
uint8_t memory_get_sound_enable(void);
uint8_t memory_get_flip_screen(void);
uint8_t memory_get_lamp1(void);
uint8_t memory_get_lamp2(void);
uint8_t memory_get_coin_lockout(void);
uint8_t memory_get_coin_counter(void);

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

// Set input ports - for external input handling
void memory_set_input_port(uint8_t port, uint8_t value);

#endif // MEMORY_H