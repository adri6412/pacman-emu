#ifndef VIDEO_H
#define VIDEO_H

#ifdef _WIN32
    #include <SDL2/SDL.h>
#else
    #include <SDL2/SDL.h>
#endif
#include <stdbool.h>
#include <stdint.h>

// Pacman video constants
#define SCREEN_WIDTH    224
#define SCREEN_HEIGHT   288
#define TILE_SIZE       8

// Sprite constants
#define MAX_SPRITES     8
#define SPRITE_WIDTH    16
#define SPRITE_HEIGHT   16

// Function prototypes
bool video_init(SDL_Renderer *renderer, int scale_factor);
void video_cleanup(void);
void video_render(void);
void video_update_palette(uint8_t index, uint8_t value);

// Debugging functions
void video_enable_debug(bool enable);
void video_draw_debug_info(void);

#endif // VIDEO_H