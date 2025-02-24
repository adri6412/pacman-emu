#include "../include/video.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declaration of debug_log function
extern void debug_log(const char *format, ...);

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
    debug_log("Initializing video hardware");
    
    renderer = r;
    scale = scale_factor;
    
    if (!renderer) {
        debug_log("ERROR: NULL renderer passed to video_init");
        return false;
    }
    
    // Initialize flip table
    init_flip_table();
    debug_log("Bit flip table initialized");
    
    // Create screen texture
    screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    
    if (!screen_texture) {
        debug_log("ERROR: Failed to create screen texture: %s", SDL_GetError());
        return false;
    }
    debug_log("Screen texture created successfully");
    
    // Create pixel buffer
    pixel_buffer = (uint32_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!pixel_buffer) {
        debug_log("ERROR: Failed to allocate pixel buffer");
        SDL_DestroyTexture(screen_texture);
        screen_texture = NULL;
        return false;
    }
    debug_log("Pixel buffer allocated successfully");
    
    // Clear pixel buffer
    memset(pixel_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    debug_log("Video initialization complete");
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

// Update palette entry based on MAME implementation
void video_update_palette(uint8_t index, uint8_t value) {
    uint32_t *palette = memory_get_palette();
    if (palette) {
        // Pacman uses a resistor network for RGB values:
        // bit 7 -- 220 ohm resistor  -- BLUE
        //       -- 470 ohm resistor  -- BLUE
        //       -- 220 ohm resistor  -- GREEN
        //       -- 470 ohm resistor  -- GREEN
        //       -- 1  kohm resistor  -- GREEN
        //       -- 220 ohm resistor  -- RED
        //       -- 470 ohm resistor  -- RED
        // bit 0 -- 1  kohm resistor  -- RED
        
        // Extract bits
        uint8_t bit0, bit1, bit2;
        
        // Red component (bits 0-2)
        bit0 = (value >> 0) & 0x01;
        bit1 = (value >> 1) & 0x01;
        bit2 = (value >> 2) & 0x01;
        uint8_t r = (bit0 * 0x21) + (bit1 * 0x47) + (bit2 * 0x97);
        
        // Green component (bits 3-5)
        bit0 = (value >> 3) & 0x01;
        bit1 = (value >> 4) & 0x01;
        bit2 = (value >> 5) & 0x01;
        uint8_t g = (bit0 * 0x21) + (bit1 * 0x47) + (bit2 * 0x97);
        
        // Blue component (bits 6-7)
        bit0 = 0;
        bit1 = (value >> 6) & 0x01;
        bit2 = (value >> 7) & 0x01;
        uint8_t b = (bit1 * 0x47) + (bit2 * 0x97);
        
        // Store RGBA value (alpha = 0xFF)
        palette[index] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
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

// Get sprite data from memory (based on MAME implementation)
static void get_sprite_data(int sprite_num, int *sprite_code, int *sprite_color, 
                          int *sprite_x, int *sprite_y, bool *flip_x, bool *flip_y) {
    // In Pacman, sprites are defined at 0x4FF0-0x4FFF (8 sprites, 2 bytes each)
    // First byte: sprite code (bits 2-7), Y flip (bit 0), X flip (bit 1)
    // Second byte: color
    uint8_t *ram = memory_get_vram(); // WorkRAM in our implementation
    
    // For debugging, return hardcoded values if RAM is not initialized properly
    if (!ram || sprite_num >= MAX_SPRITES) {
        *sprite_code = sprite_num;
        *sprite_color = 1;
        *sprite_x = 100 + sprite_num * 20;
        *sprite_y = 100;
        *flip_x = false;
        *flip_y = false;
        return;
    }
    
    // Check if we're near the end of VRAM (might be out of bounds)
    if (VRAM_SIZE < 0xFF0 + (MAX_SPRITES * 2)) {
        // Hardcoded values for safety
        *sprite_code = sprite_num;
        *sprite_color = 1;
        *sprite_x = 100 + sprite_num * 20;
        *sprite_y = 100;
        *flip_x = false;
        *flip_y = false;
        return;
    }
    
    uint16_t sprites_offset = 0xFF0; // Offset in VRAM where sprite data is stored
    uint8_t sprite_attr1 = ram[sprites_offset + sprite_num * 2];
    uint8_t sprite_attr2 = ram[sprites_offset + sprite_num * 2 + 1];
    
    // From MAME: "sprite number (bits 2-7), Y flip (bit 0), X flip (bit 1)"
    *sprite_code = sprite_attr1 >> 2;
    *flip_y = (sprite_attr1 & 0x01) != 0;
    *flip_x = (sprite_attr1 & 0x02) != 0;
    
    // Color information
    *sprite_color = sprite_attr2 & 0x3F;
    
    // Sprite coordinates come from I/O ports 0x5060-0x506F (x,y pairs for 8 sprites)
    // For now use fixed values while we debug the port issues
    *sprite_x = 100 + (sprite_num * 20);
    *sprite_y = 100;
    
    // Original MAME-style code (commented for debugging)
    // uint8_t port_offset = 0x60;  // Address 0x5060 in I/O space
    // *sprite_x = io_read_byte(port_offset + sprite_num * 2);
    // *sprite_y = 272 - io_read_byte(port_offset + sprite_num * 2 + 1); // Y is inverted
    
    // Adjust for hardware quirks - MAME subtracts 16 from sprite X position
    *sprite_x -= 16;
}

// Render the current frame (based on MAME implementation)
void video_render(void) {
    // Clear screen to black
    memset(pixel_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    // Get pointers to video memory
    uint8_t *vram = memory_get_vram();
    uint8_t *cram = memory_get_cram();
    
    if (!vram || !cram) {
        printf("WARNING: Video memory not initialized, rendering test pattern\n");
        // If video memory is not available, just draw a test pattern
        draw_test_pattern();
        return;
    }
    
    // Check if screen is flipped (temporarily disabled for debugging)
    bool flip = false; // memory_get_flip_screen() != 0;
    
    // Draw background tilemap (28x36 tiles)
    for (int ty = 0; ty < TILE_ROWS; ty++) {
        for (int tx = 0; tx < TILE_COLUMNS; tx++) {
            // Compute tile index with correct memory layout (32 columns in memory)
            int tile_index = ty * 32 + tx;
            
            // Apply screen flipping if needed
            int screen_x, screen_y;
            if (flip) {
                screen_x = (TILE_COLUMNS - 1 - tx) * TILE_SIZE;
                screen_y = (TILE_ROWS - 1 - ty) * TILE_SIZE;
            } else {
                screen_x = tx * TILE_SIZE;
                screen_y = ty * TILE_SIZE;
            }
            
            if (tile_index < VRAM_SIZE) {
                uint8_t tile = vram[tile_index];
                uint8_t color = cram[tile_index];
                
                // Apply palette bank (used by some games)
                // color |= (charbank << 5) | (palettebank << 6);
                
                // Draw the tile character
                draw_character(screen_x, screen_y, tile, color);
            }
        }
    }
    
    // Draw sprites using hardware registers
    // Pacman has 8 16x16 sprites
    // In order of priority (lowest number = highest priority)
    for (int i = 0; i < MAX_SPRITES; i++) {
        int sprite_code, sprite_color, sprite_x, sprite_y;
        bool flip_x, flip_y;
        
        // Read sprite data from memory
        get_sprite_data(i, &sprite_code, &sprite_color, &sprite_x, &sprite_y, &flip_x, &flip_y);
        
        // Apply screen flip if needed
        if (flip) {
            sprite_x = SCREEN_WIDTH - sprite_x - SPRITE_WIDTH;
            sprite_y = SCREEN_HEIGHT - sprite_y - SPRITE_HEIGHT;
            flip_x = !flip_x;
            flip_y = !flip_y;
        }
        
        // Draw the sprite if it's visible on screen
        if (sprite_x >= -SPRITE_WIDTH && sprite_x < SCREEN_WIDTH &&
            sprite_y >= -SPRITE_HEIGHT && sprite_y < SCREEN_HEIGHT) {
            draw_sprite(sprite_x, sprite_y, sprite_code, sprite_color, flip_x, flip_y);
        }
    }
    
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

// Draw a test pattern to show something when VRAM is not available
static void draw_test_pattern(void) {
    // Draw a colored checkerboard pattern
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int cell_x = x / 16;
            int cell_y = y / 16;
            int color_index = (cell_x + cell_y) % 4;
            
            // Some vibrant colors
            uint32_t colors[4] = {
                0xFF0000FF,  // Red
                0xFF00FF00,  // Green
                0xFFFF0000,  // Blue
                0xFFFFFF00   // Yellow
            };
            
            pixel_buffer[y * SCREEN_WIDTH + x] = colors[color_index];
        }
    }
    
    // Draw text "PACMAN EMU" in the center
    // Simple font rendering...
    const char* text = "PACMAN EMU";
    int text_x = (SCREEN_WIDTH - (strlen(text) * 8)) / 2;
    int text_y = SCREEN_HEIGHT / 2 - 4;
    
    // Draw each character (very simple, just rectangles for demonstration)
    for (int i = 0; text[i] != '\0'; i++) {
        int x = text_x + i * 8;
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 8; px++) {
                // Simple shapes for letters (just rectangles with holes)
                if (px > 0 && px < 7 && py > 0 && py < 7) {
                    continue;  // Hollow center
                }
                
                if (x + px >= 0 && x + px < SCREEN_WIDTH && 
                    text_y + py >= 0 && text_y + py < SCREEN_HEIGHT) {
                    pixel_buffer[(text_y + py) * SCREEN_WIDTH + (x + px)] = 0xFFFFFFFF;  // White
                }
            }
        }
    }
}

// Draw debug information (enhanced for MAME-style debugging)
void video_draw_debug_info(void) {
    // Draw a grid to show tile boundaries
    for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            pixel_buffer[y * SCREEN_WIDTH + x] = 0xFFFF0000;  // Red
        }
    }
    
    for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            pixel_buffer[y * SCREEN_WIDTH + x] = 0xFFFF0000;  // Red
        }
    }
    
    // Draw sprite boundaries for debugging
    for (int i = 0; i < MAX_SPRITES; i++) {
        int sprite_code, sprite_color, sprite_x, sprite_y;
        bool flip_x, flip_y;
        
        get_sprite_data(i, &sprite_code, &sprite_color, &sprite_x, &sprite_y, &flip_x, &flip_y);
        
        // Draw sprite bounds if visible
        if (sprite_x >= -SPRITE_WIDTH && sprite_x < SCREEN_WIDTH &&
            sprite_y >= -SPRITE_HEIGHT && sprite_y < SCREEN_HEIGHT) {
            
            // Draw sprite outline
            uint32_t outline_color = 0xFF00FF00; // Green
            
            for (int x = 0; x < SPRITE_WIDTH; x++) {
                int top_y = sprite_y;
                int bottom_y = sprite_y + SPRITE_HEIGHT - 1;
                
                if (top_y >= 0 && top_y < SCREEN_HEIGHT) {
                    int px = sprite_x + x;
                    if (px >= 0 && px < SCREEN_WIDTH) {
                        pixel_buffer[top_y * SCREEN_WIDTH + px] = outline_color;
                    }
                }
                
                if (bottom_y >= 0 && bottom_y < SCREEN_HEIGHT) {
                    int px = sprite_x + x;
                    if (px >= 0 && px < SCREEN_WIDTH) {
                        pixel_buffer[bottom_y * SCREEN_WIDTH + px] = outline_color;
                    }
                }
            }
            
            for (int y = 0; y < SPRITE_HEIGHT; y++) {
                int left_x = sprite_x;
                int right_x = sprite_x + SPRITE_WIDTH - 1;
                
                if (left_x >= 0 && left_x < SCREEN_WIDTH) {
                    int py = sprite_y + y;
                    if (py >= 0 && py < SCREEN_HEIGHT) {
                        pixel_buffer[py * SCREEN_WIDTH + left_x] = outline_color;
                    }
                }
                
                if (right_x >= 0 && right_x < SCREEN_WIDTH) {
                    int py = sprite_y + y;
                    if (py >= 0 && py < SCREEN_HEIGHT) {
                        pixel_buffer[py * SCREEN_WIDTH + right_x] = outline_color;
                    }
                }
            }
            
            // Draw sprite number in the top-left corner of the sprite
            if (sprite_x >= 0 && sprite_x + 8 < SCREEN_WIDTH &&
                sprite_y >= 0 && sprite_y + 8 < SCREEN_HEIGHT) {
                // Simple way to draw a number (not very pretty but works for debug)
                for (int dx = 0; dx < 5; dx++) {
                    for (int dy = 0; dy < 7; dy++) {
                        if (sprite_x + dx + 1 >= 0 && sprite_x + dx + 1 < SCREEN_WIDTH &&
                            sprite_y + dy + 1 >= 0 && sprite_y + dy + 1 < SCREEN_HEIGHT) {
                            // Very basic font rendering for debug - just shows the sprite number
                            if ((i == 0 && (dx == 1 || dx == 2 || dx == 3) && (dy == 0 || dy == 6)) ||
                                (i == 0 && (dx == 0 || dx == 4) && (dy >= 1 && dy <= 5))) {
                                pixel_buffer[(sprite_y + dy + 1) * SCREEN_WIDTH + (sprite_x + dx + 1)] = 0xFFFFFF00;
                            } else if (i == 1 && (dx == 2 && dy <= 6)) {
                                pixel_buffer[(sprite_y + dy + 1) * SCREEN_WIDTH + (sprite_x + dx + 1)] = 0xFFFFFF00;
                            } else if (i >= 2) {
                                // Just a dot for other sprites to keep it simple
                                if (dx == 2 && dy == 2) {
                                    pixel_buffer[(sprite_y + dy + 1) * SCREEN_WIDTH + (sprite_x + dx + 1)] = 0xFFFFFF00;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}