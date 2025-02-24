#ifndef VIDEO_H
#define VIDEO_H

#ifdef _WIN32
    #include <SDL2/SDL.h>
#else
    #include <SDL2/SDL.h>
#endif
#include <stdbool.h>
#include <stdint.h>

// Pacman video constants (based on MAME implementation)
#define SCREEN_WIDTH    224
#define SCREEN_HEIGHT   288
#define TILE_SIZE       8
#define TILE_COLUMNS    28  // 28 columns of 8-pixel tiles
#define TILE_ROWS       36  // 36 rows of 8-pixel tiles

// Sprite constants
#define MAX_SPRITES     8
#define SPRITE_WIDTH    16
#define SPRITE_HEIGHT   16

// Color constants
#define PALETTE_ENTRIES 32      // 32 colors in palette PROM
#define COLORTABLE_SIZE 256     // 256 entries in the color lookup PROM

// Hardware registers
#define REG_CHAR_BANK   0       // Character bank (used by some games)
#define REG_SPRITE_BANK 0       // Sprite bank (used by some games)
#define REG_PALETTE_BANK 0      // Palette bank (used by some games)
#define REG_FLIP_SCREEN 0       // Flip screen bit

// Function prototypes
bool video_init(SDL_Renderer *renderer, int scale_factor);
void video_cleanup(void);
void video_render(void);
void video_update_palette(uint8_t index, uint8_t value);

// Debugging functions
void video_enable_debug(bool enable);
void video_draw_debug_info(void);

#endif // VIDEO_H