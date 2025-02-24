#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <SDL2/SDL.h>
    #include <direct.h>
    #define PATH_SEPARATOR '\\'
    #define mkdir(dir, mode) _mkdir(dir)
#else
    #include <SDL2/SDL.h>
    #include <sys/stat.h>
    #define PATH_SEPARATOR '/'
#endif

#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/video.h"
#include "../include/input.h"

#define WINDOW_WIDTH 224
#define WINDOW_HEIGHT 288
#define SCALE_FACTOR 2

// Print usage information
void print_usage(const char *program_name) {
    printf("Pacman Emulator\n");
    printf("Usage: %s [options] [rom_path]\n\n", program_name);
    printf("Options:\n");
    printf("  --help                Show this help message\n");
    printf("  --test                Use built-in test ROM (no external ROM needed)\n");
    printf("\n");
    printf("If rom_path is a directory, it will be treated as a MAME ROM set directory.\n");
    printf("If rom_path is a file, it will be loaded as a single ROM file.\n");
}

int main(int argc, char *argv[]) {
    // Enable console output for Windows
    #ifdef _WIN32
    // Redirect console output to terminal
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    #endif
    
    // Initialize debug logging
    debug_log("Pacman Emulator starting up");
    debug_log("Command line: %s", argv[0]);
    
    // Default settings
    const char *rom_path = NULL;
    bool use_test_rom = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--test") == 0) {
            use_test_rom = true;
        } else if (argv[i][0] != '-') {
            rom_path = argv[i];
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Check if we have a ROM path or using test ROM
    if (!rom_path && !use_test_rom) {
        printf("Error: No ROM path specified.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // If using test ROM, set the path to our built-in test ROM
    if (use_test_rom) {
        rom_path = "data/test.rom";
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Pacman Emulator",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH * SCALE_FACTOR, WINDOW_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize emulator components
    if (!memory_init(rom_path)) {
        printf("Failed to load ROM: %s\n", rom_path);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    cpu_init();
    video_init(renderer, SCALE_FACTOR);
    input_init();
    
    // Main emulation loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        // Handle input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            input_process_event(&event);
        }
        
        // Execute CPU cycles
        cpu_execute_frame();
        
        // Render screen
        video_render();
        SDL_RenderPresent(renderer);
        
        // Cap at ~60fps
        SDL_Delay(16);
    }
    
    // Cleanup
    video_cleanup();
    memory_cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}