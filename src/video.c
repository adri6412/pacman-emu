#include "../include/video.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Video hardware state
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
static uint32_t *pixel_buffer = NULL;
static int scale = 2;
static bool debug_mode = false;

// Sprite data flipping tables
static uint8_t flip_table[256];
static bool flip_table_initialized = false;

// Initialize the bit flip table
static void init_flip_table(void) {
    if (flip_table_initialized) return;
    
    for (int i = 0; i < 256; i++) {
        uint8_t flipped = 0;
        for (int bit = 0; bit < 8; bit++) {
            if (i & (1 << bit)) {
                flipped |= (1 << (7 - bit));
            }
        }
        flip_table[i] = flipped;
    }
    
    flip_table_initialized = true;
}

// Initialize video hardware
bool video_init(SDL_Renderer *r, int scale_factor) {
    renderer = r;
    scale = scale_factor;
    
    // Initialize flip table
    init_flip_table();
    
    // Create screen texture
    screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    
    if (!screen_texture) {
        fprintf(stderr, "Failed to create screen texture: %s\n", SDL_GetError());
        return false;
    }
    
    // Create pixel buffer
    pixel_buffer = (uint32_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!pixel_buffer) {
        fprintf(stderr, "Failed to allocate pixel buffer\n");
        SDL_DestroyTexture(screen_texture);
        screen_texture = NULL;
        return false;
    }
    
    // Clear pixel buffer
    memset(pixel_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    return true;
}

// Clean up video resources
void video_cleanup(void) {
    if (pixel_buffer) {
        free(pixel_buffer);
        pixel_buffer = NULL;
    }
    
    if (screen_texture) {
        SDL_DestroyTexture(screen_texture);
        screen_texture = NULL;
    }
    
    renderer = NULL;
}

// Update palette entry
void video_update_palette(uint8_t index, uint8_t value) {
    // In a real Pacman emulator, this would handle palette updates
    // For simplicity, we're using a fixed palette in this example
}

// Enable/disable debug visualization
void video_enable_debug(bool enable) {
    debug_mode = enable;
}

// Draw a character from the character ROM
static void draw_character(int x, int y, uint8_t character, uint8_t color) {
    uint8_t *charset = memory_get_charset();
    uint32_t *palette = memory_get_palette();
    
    if (!charset || !palette) return;
    
    uint8_t *char_data = &charset[character * 8];
    
    for (int cy = 0; cy < 8; cy++) {
        uint8_t row = char_data[cy];
        
        for (int cx = 0; cx < 8; cx++) {
            if (row & (0x80 >> cx)) {
                // Character pixel is set, use foreground color
                int pixel_x = x + cx;
                int pixel_y = y + cy;
                
                if (pixel_x >= 0 && pixel_x < SCREEN_WIDTH && 
                    pixel_y >= 0 && pixel_y < SCREEN_HEIGHT) {
                    pixel_buffer[pixel_y * SCREEN_WIDTH + pixel_x] = palette[color & 0x0F];
                }
            }
        }
    }
}

// Draw a sprite
static void draw_sprite(int x, int y, int sprite_index, uint8_t color_index, bool h_flip, bool v_flip) {
    uint8_t *sprites = memory_get_spritedata();
    uint32_t *palette = memory_get_palette();
    
    if (!sprites || !palette) return;
    
    // Pacman sprites are 16x16 pixels, stored as 64 bytes per sprite
    // Each 16x16 sprite is made up of four 8x8 tiles
    uint8_t *sprite = &sprites[sprite_index * 16];
    
    for (int sy = 0; sy < SPRITE_HEIGHT; sy++) {
        int y_offset = v_flip ? (15 - sy) : sy;
        
        // Determine which 8x8 tile we're in
        int tile_y = y_offset / 8;
        int tile_row = y_offset % 8;
        
        for (int sx = 0; sx < SPRITE_WIDTH; sx++) {
            int x_offset = h_flip ? (15 - sx) : sx;
            
            // Determine which 8x8 tile we're in
            int tile_x = x_offset / 8;
            int tile_col = x_offset % 8;
            
            // Calculate index into sprite data
            int tile_index = tile_y * 2 + tile_x;
            uint8_t tile_byte = sprite[tile_index * 8 + tile_row];
            
            // Check if the pixel is set
            bool pixel_set = (tile_byte & (0x80 >> tile_col)) != 0;
            
            if (pixel_set) {
                int screen_x = x + sx;
                int screen_y = y + sy;
                
                if (screen_x >= 0 && screen_x < SCREEN_WIDTH && 
                    screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                    pixel_buffer[screen_y * SCREEN_WIDTH + screen_x] = palette[color_index & 0x0F];
                }
            }
        }
    }
}

// Render the current frame
void video_render(void) {
    // Clear screen to black
    memset(pixel_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    // Get pointers to video memory
    uint8_t *vram = memory_get_vram();
    uint8_t *cram = memory_get_cram();
    
    if (!vram || !cram) return;
    
    // Draw background tilemap (28x36 tiles)
    for (int ty = 0; ty < 36; ty++) {
        for (int tx = 0; tx < 28; tx++) {
            int tile_index = ty * 32 + tx; // Pacman video memory has 32 columns
            int tile_x = tx * 8;
            int tile_y = ty * 8;
            
            if (tile_index < VRAM_SIZE) {
                uint8_t tile = vram[tile_index];
                uint8_t color = cram[tile_index];
                
                draw_character(tile_x, tile_y, tile, color);
            }
        }
    }
    
    // In a real Pacman, sprites would be read from sprite registers
    // For this simplified version, we'll hard-code some sprites
    
    // Pac-Man sprite at center of screen
    draw_sprite(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 8, 0, 6, false, false); // Yellow
    
    // Draw ghost sprites around Pacman
    draw_sprite(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 8, 1, 4, false, false); // Red ghost (Blinky)
    draw_sprite(SCREEN_WIDTH / 2 + 24, SCREEN_HEIGHT / 2 - 8, 2, 1, false, false); // Blue ghost (Inky)
    draw_sprite(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 40, 3, 2, false, false); // Green ghost (Pinky)
    draw_sprite(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 + 24, 4, 5, false, false); // Orange ghost (Clyde)
    
    // Draw debug information if enabled
    if (debug_mode) {
        video_draw_debug_info();
    }
    
    // Update the screen texture
    SDL_UpdateTexture(screen_texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t));
    
    // Clear the renderer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw the texture scaled to window size
    SDL_Rect dest_rect = {0, 0, SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale};
    SDL_RenderCopy(renderer, screen_texture, NULL, &dest_rect);
}

// Draw debug information
void video_draw_debug_info(void) {
    // Draw a simple grid pattern to show tile boundaries
    for (int y = 0; y < SCREEN_HEIGHT; y += 8) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            pixel_buffer[y * SCREEN_WIDTH + x] = 0x444444FF;
        }
    }
    
    for (int x = 0; x < SCREEN_WIDTH; x += 8) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            pixel_buffer[y * SCREEN_WIDTH + x] = 0x444444FF;
        }
    }
}