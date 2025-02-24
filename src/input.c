#include "../include/input.h"
#include <stdio.h>

// Input state
static uint8_t input_port1 = 0xFF; // Active low logic (0 = pressed, 1 = released)
static uint8_t input_port2 = 0xFF;

// Key mappings
static const struct {
    SDL_Keycode key;
    uint8_t *port;
    uint8_t mask;
} key_mappings[] = {
    // Player 1 controls
    {SDLK_UP,     &input_port1, INPUT_P1_UP},
    {SDLK_LEFT,   &input_port1, INPUT_P1_LEFT},
    {SDLK_RIGHT,  &input_port1, INPUT_P1_RIGHT},
    {SDLK_DOWN,   &input_port1, INPUT_P1_DOWN},
    
    // Player 2 controls (WASD)
    {SDLK_w,      &input_port2, INPUT_P2_UP},
    {SDLK_a,      &input_port2, INPUT_P2_LEFT},
    {SDLK_d,      &input_port2, INPUT_P2_RIGHT},
    {SDLK_s,      &input_port2, INPUT_P2_DOWN},
    
    // System controls
    {SDLK_5,      &input_port1, INPUT_COIN},      // Insert coin
    {SDLK_1,      &input_port1, INPUT_P1_START},  // Player 1 start
    {SDLK_2,      &input_port1, INPUT_P2_START},  // Player 2 start
    {SDLK_F1,     &input_port1, INPUT_SERVICE},   // Service button
    
    // Terminator
    {SDLK_UNKNOWN, NULL, 0}
};

// Initialize input system
void input_init(void) {
    input_reset();
}

// Reset input state
void input_reset(void) {
    input_port1 = 0xFF;
    input_port2 = 0xFF;
}

// Process an SDL input event
void input_process_event(SDL_Event *event) {
    if (!event) return;
    
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            {
                SDL_KeyboardEvent *key_event = &event->key;
                SDL_Keycode keycode = key_event->keysym.sym;
                bool pressed = (event->type == SDL_KEYDOWN);
                
                // Find the key in our mapping table
                for (int i = 0; key_mappings[i].port != NULL; i++) {
                    if (key_mappings[i].key == keycode) {
                        if (pressed) {
                            // Key pressed - clear the bit (active low)
                            *(key_mappings[i].port) &= ~key_mappings[i].mask;
                        } else {
                            // Key released - set the bit
                            *(key_mappings[i].port) |= key_mappings[i].mask;
                        }
                        break;
                    }
                }
            }
            break;
            
        // Add joystick/gamepad support here if needed
    }
}

// Read input port 1 (player 1 controls, coins, start buttons)
uint8_t input_read_port1(void) {
    return input_port1;
}

// Read input port 2 (player 2 controls, DIP switches)
uint8_t input_read_port2(void) {
    return input_port2;
}