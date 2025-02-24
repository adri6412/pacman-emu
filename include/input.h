#ifndef INPUT_H
#define INPUT_H

#ifdef _WIN32
    #include <SDL2/SDL.h>
#else
    #include <SDL2/SDL.h>
#endif
#include <stdbool.h>
#include <stdint.h>

// Input port definitions
#define INPUT_P1_UP     0x01
#define INPUT_P1_LEFT   0x02
#define INPUT_P1_RIGHT  0x04
#define INPUT_P1_DOWN   0x08
#define INPUT_COIN      0x10
#define INPUT_P1_START  0x20
#define INPUT_P2_START  0x40
#define INPUT_SERVICE   0x80

#define INPUT_P2_UP     0x01
#define INPUT_P2_LEFT   0x02
#define INPUT_P2_RIGHT  0x04
#define INPUT_P2_DOWN   0x08
#define INPUT_DIP3      0x10
#define INPUT_DIP4      0x20
#define INPUT_DIP5      0x40
#define INPUT_DIP6      0x80

// Function prototypes
void input_init(void);
void input_process_event(SDL_Event *event);
uint8_t input_read_port1(void);
uint8_t input_read_port2(void);
void input_reset(void);

#endif // INPUT_H