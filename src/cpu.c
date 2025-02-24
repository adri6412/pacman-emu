#include "../include/cpu.h"
#include "../include/memory.h"
#include "../src/z80/z80.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>  // For SDL_Delay

// External declaration of debug_log function
extern void debug_log(const char *format, ...);

// Z80 CPU instance
static z80 cpu;
static bool interrupt_pending = false;

// Lookup tables for instruction timing
static const uint8_t cycle_counts[256] = {
    4, 10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4, 7, 4,
    8, 10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
    7, 10, 16, 6, 4, 4, 7, 4, 7, 11, 16, 6, 4, 4, 7, 4,
    7, 10, 13, 6, 11, 11, 10, 4, 7, 11, 13, 6, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5, 10, 10, 10, 10, 11, 7, 11, 5, 10, 10, 0, 10, 17, 7, 11,
    5, 10, 10, 11, 10, 11, 7, 11, 5, 4, 10, 11, 10, 0, 7, 11,
    5, 10, 10, 19, 10, 11, 7, 11, 5, 4, 10, 4, 10, 0, 7, 11,
    5, 10, 10, 4, 10, 11, 7, 11, 5, 6, 10, 4, 10, 0, 7, 11
};

// Memory read callback for Z80
static uint8_t cpu_read_callback(void* userdata, uint16_t address) {
    return memory_read_byte(address);
}

// Memory write callback for Z80
static void cpu_write_callback(void* userdata, uint16_t address, uint8_t data) {
    memory_write_byte(address, data);
}

// IO read callback for Z80
static uint8_t cpu_port_in(z80* const z, uint8_t port) {
    return io_read_byte(port);
}

// IO write callback for Z80
static void cpu_port_out(z80* const z, uint8_t port, uint8_t data) {
    io_write_byte(port, data);
}

// CPU initialization
void cpu_init(void) {
    // Initialize the Z80 CPU
    z80_init(&cpu);
    
    // Set up memory and IO callbacks
    cpu.read_byte = cpu_read_callback;
    cpu.write_byte = cpu_write_callback;
    cpu.port_in = cpu_port_in;
    cpu.port_out = cpu_port_out;
    cpu.userdata = NULL; // We don't need any user data for now
    
    debug_log("Z80 CPU initialized using superzazu's Z80");
}

// Reset the CPU to initial state
void cpu_reset(void) {
    // Initialize the Z80 CPU - this also resets the CPU
    z80_init(&cpu);
    
    // Set up callbacks again (in case they were lost during init)
    cpu.read_byte = cpu_read_callback;
    cpu.write_byte = cpu_write_callback;
    cpu.port_in = cpu_port_in;
    cpu.port_out = cpu_port_out;
    
    // Set some initial values
    cpu.pc = 0;         // Start at address 0 (ROM)
    cpu.sp = 0xF000;    // Initial stack pointer in high RAM
    
    interrupt_pending = false;
    
    debug_log("Z80 CPU reset");
}

// Read byte from memory
uint8_t cpu_read_byte(uint16_t address) {
    return memory_read_byte(address);
}

// Write byte to memory
void cpu_write_byte(uint16_t address, uint8_t value) {
    memory_write_byte(address, value);
}

// Read next byte from PC
static uint8_t fetch_byte(void) {
    uint8_t value = cpu_read_byte(regs.pc);
    regs.pc++;
    return value;
}

// Read next word from PC
static uint16_t fetch_word(void) {
    uint16_t value = cpu_read_byte(regs.pc) | (cpu_read_byte(regs.pc + 1) << 8);
    regs.pc += 2;
    return value;
}

// Push word to stack
static void push(uint16_t value) {
    regs.sp -= 2;
    memory_write_word(regs.sp, value);
}

// Pop word from stack
static uint16_t pop(void) {
    uint16_t value = memory_read_word(regs.sp);
    regs.sp += 2;
    return value;
}

// Handle interrupt request
void cpu_interrupt(void) {
    if (cpu.iff1) {
        // Generate an interrupt with data 0xFF (RST 38h)
        z80_gen_int(&cpu, 0xFF);
        debug_log("Z80 interrupt requested");
    }
}

// Process a pending interrupt - this is handled by libz80 now
static void handle_interrupt(void) {
    // Nothing to do here, libz80 handles interrupts internally
}

// Execute CPU instructions for one frame (~16.6ms)
void cpu_execute_frame(void) {
    // Pacman Z80 runs at 3.072 MHz, so one frame is about 50,000 cycles
    const uint32_t CYCLES_PER_FRAME = 50000;
    
    // Initialize test pattern at first run
    static bool initialized_vram = false;
    static bool first_execution = true;
    
    if (first_execution) {
        first_execution = false;
        debug_log("ROM starting bytes: %02X %02X %02X %02X", 
                 cpu_read_callback(NULL, 0), cpu_read_callback(NULL, 1),
                 cpu_read_callback(NULL, 2), cpu_read_callback(NULL, 3));
        debug_log("Z80 emulation starting");
    }
    
    if (!initialized_vram) {
        debug_log("Initializing test pattern in VRAM");
        
        // Get VRAM and CRAM memory
        uint8_t *vram = memory_get_vram();
        uint8_t *cram = memory_get_cram();
        
        if (vram && cram) {
            // First, clear VRAM and CRAM to all zeros
            memset(vram, 0, 0x0400);  // Clear 1KB VRAM
            memset(cram, 0, 0x0400);  // Clear 1KB CRAM
            
            // Create a simple Pacman-like maze pattern in VRAM
            for (int y = 0; y < 36; y++) {
                for (int x = 0; x < 28; x++) {
                    int tile_index = y * 32 + x; // 32 columns in VRAM
                    
                    // Border around the screen
                    if (x == 0 || x == 27 || y == 0 || y == 35) {
                        vram[tile_index] = 0x01; // Use tile 1 for walls
                        cram[tile_index] = 0x07; // Green color for walls
                    }
                    // Add some internal walls
                    else if ((x % 4 == 0 && y % 4 == 0) || 
                             (x % 8 == 0 && y % 8 == 0)) {
                        vram[tile_index] = 0x01; // Walls
                        cram[tile_index] = 0x07; // Green
                    }
                    // Add some dots
                    else if ((x + y) % 4 == 0) {
                        vram[tile_index] = 0x02; // Dots
                        cram[tile_index] = 0x06; // White
                    }
                    // Add some other elements
                    else if ((x * y) % 16 == 0) {
                        vram[tile_index] = 0x03; // Fruit or other elements
                        cram[tile_index] = (x + y) % 8; // Various colors
                    }
                    // Black background elsewhere
                    else {
                        vram[tile_index] = 0x00; // Empty
                        cram[tile_index] = 0x00; // Black
                    }
                }
            }
            
            // Add "HELLO WORLD" to the center of the screen
            const uint8_t hello_world[] = {
                0x01, 0x02, 0x03, 0x03, 0x04, 0x00, 
                0x05, 0x04, 0x06, 0x03, 0x07  // HELLO WORLD
            };
            
            int text_y = 15;  // Middle of screen
            int text_x = 9;   // Center horizontally
            
            for (int i = 0; i < sizeof(hello_world); i++) {
                int idx = text_y * 32 + text_x + i;
                vram[idx] = hello_world[i];
                cram[idx] = (i < 5) ? 0x05 : 0x01;  // Yellow for HELLO, Blue for WORLD
            }
            
            // Add Pacman and ghosts to specific positions
            int pacman_pos = 20 * 32 + 14;
            vram[pacman_pos] = 0x04; // Pacman character
            cram[pacman_pos] = 0x05; // Yellow
            
            // Add ghosts at different positions
            int ghost_positions[4] = {
                12 * 32 + 10, // Ghost 1
                12 * 32 + 17, // Ghost 2
                18 * 32 + 10, // Ghost 3
                18 * 32 + 17  // Ghost 4
            };
            
            for (int i = 0; i < 4; i++) {
                vram[ghost_positions[i]] = 0x05 + i; // Ghost characters
                cram[ghost_positions[i]] = i + 1; // Different colors for each ghost
            }
            
            // Set sprite data for the 8 sprites (in VRAM at 0x4FF0-0x4FFF)
            // Each sprite has 2 bytes: sprite number, color
            for (int i = 0; i < 8; i++) {
                vram[0xFF0 + i*2] = (i << 2); // sprite number, no flip
                vram[0xFF1 + i*2] = i % 7 + 1; // color (1-7, avoid black)
            }
            
            // Set sprite positions (in I/O ports 0x5060-0x506F)
            for (int i = 0; i < 8; i++) {
                memory_set_input_port(0x60 + i*2, 100 + i*20); // X positions
                memory_set_input_port(0x61 + i*2, 100 + i*20); // Y positions
            }
            
            debug_log("VRAM test pattern initialized");
            initialized_vram = true;
        } else {
            debug_log("ERROR: Could not get VRAM/CRAM pointers");
        }
    }
    
    // Try to execute the ROM code if we haven't done so yet
    if (!executed_rom) {
        debug_log("Attempting to execute ROM code");
        
        // Set PC to beginning of ROM
        regs.pc = 0x0000;
        
        // Print the first few bytes of the ROM to check if it's valid
        uint8_t *rom_data = memory_get_vram() - 0x4000;  // Crude approximation to get ROM address
        debug_log("ROM starting bytes: %02X %02X %02X %02X", 
                 cpu_read_byte(0), cpu_read_byte(1), cpu_read_byte(2), cpu_read_byte(3));
        
        // Enable interrupts for the game to run
        regs.iff1 = true;
        regs.iff2 = true;
        
        // Mark as executed so we don't try again
        executed_rom = true;
        debug_log("ROM execution initialized");
    }
    
    // Now execute some CPU cycles for this frame
    const uint32_t CYCLES_PER_FRAME = 50000; // Pacman Z80 runs at 3.072 MHz (50000 cycles/frame)
    uint32_t executed_cycles = 0;
    
    // Execute Z80 instructions until we reach the required number of cycles
    unsigned long start_cyc = cpu.cyc;
    
    while ((cpu.cyc - start_cyc) < CYCLES_PER_FRAME) {
        // Execute one Z80 instruction
        z80_step(&cpu);
        
        // Avoid infinite loops by limiting iterations
        if ((cpu.cyc - start_cyc) > CYCLES_PER_FRAME * 2) {
            debug_log("Warning: Breaking out of CPU loop - too many cycles");
            break;
        }
    }
    
    executed_cycles = cpu.cyc - start_cyc;
    
    // Add a debugging log every 60 frames
    static int frame_counter = 0;
    frame_counter++;
    
    if (frame_counter % 60 == 0) {
        debug_log("Z80 PC=0x%04X, SP=0x%04X, A=0x%02X executed %lu cycles",
                cpu.pc, cpu.sp, cpu.a, executed_cycles);
    }
    
    // Get VRAM and CRAM for display
    uint8_t *vram = memory_get_vram();
    uint8_t *cram = memory_get_cram();
    
    if (vram && cram) {
        // Set up a more elaborate test pattern that looks like Pacman
        static bool first_frame = true;
        
        if (first_frame) {
            debug_log("Creating elaborate Pacman test display");
            first_frame = false;
            
            // Clear VRAM and CRAM
            memset(vram, 0, 0x0400);
            memset(cram, 0, 0x0400);
            
            // Create a maze pattern
            for (int y = 0; y < 36; y++) {
                for (int x = 0; x < 28; x++) {
                    int idx = y * 32 + x;  // VRAM layout is 32 columns wide
                    
                    // Border
                    if (x == 0 || x == 27 || y == 0 || y == 35) {
                        vram[idx] = 0x01;  // Wall character
                        cram[idx] = 0x01;  // Blue
                    }
                    // Grid pattern for maze (vertical lines)
                    else if (x % 4 == 0 && y > 5 && y < 30) {
                        vram[idx] = 0x01;  // Wall character
                        cram[idx] = 0x01;  // Blue
                    }
                    // Grid pattern for maze (horizontal lines)
                    else if (y % 4 == 0 && x > 5 && x < 22) {
                        vram[idx] = 0x01;  // Wall character
                        cram[idx] = 0x01;  // Blue
                    }
                    // Dots everywhere else with some spacing
                    else if ((x + y) % 2 == 0) {
                        vram[idx] = 0x02;  // Dot character
                        cram[idx] = 0x06;  // White/yellow
                    }
                }
            }
            
            // Add "HELLO WORLD" message in center of screen
            const uint8_t hello_world[] = {
                0x01, 0x02, 0x03, 0x03, 0x04, 0x00, 
                0x05, 0x04, 0x06, 0x03, 0x07  // HELLO WORLD
            };
            
            int text_y = 8;   // Top section of screen
            int text_x = 9;   // Center horizontally
            
            // Write "HELLO WORLD" in yellow
            for (int i = 0; i < sizeof(hello_world); i++) {
                int idx = text_y * 32 + text_x + i;
                vram[idx] = hello_world[i];
                cram[idx] = 0x05;  // Yellow
            }
            
            // Add Pacman and ghosts in specific positions
            // Pacman in bottom half
            int pacman_y = 20;
            int pacman_x = 14;
            vram[pacman_y * 32 + pacman_x] = 0x04;  // Pacman character
            cram[pacman_y * 32 + pacman_x] = 0x05;  // Yellow
            
            // Ghosts in different positions
            int ghost_positions[4][2] = {
                {12, 10},  // Ghost 1 (red)
                {12, 17},  // Ghost 2 (pink)
                {22, 10},  // Ghost 3 (cyan)
                {22, 17}   // Ghost 4 (orange)
            };
            
            uint8_t ghost_colors[4] = {0x01, 0x03, 0x02, 0x04};  // Red, Pink, Cyan, Orange
            
            for (int i = 0; i < 4; i++) {
                int idx = ghost_positions[i][0] * 32 + ghost_positions[i][1];
                vram[idx] = 0x05 + i;  // Ghost characters
                cram[idx] = ghost_colors[i];
            }
            
            // Set up sprites in VRAM
            // In Pacman, sprites data is at 0x4FF0-0x4FFF (last 16 bytes of VRAM)
            // Each sprite is 2 bytes: sprite number + flags, color
            for (int i = 0; i < 8; i++) {
                if (i < 5) {  // Only 5 sprites: Pacman + 4 ghosts
                    vram[0xFF0 + i*2] = i << 2;  // Sprite number, no flip flags
                    vram[0xFF1 + i*2] = (i == 0) ? 0x05 : ghost_colors[i-1];  // Color (Pacman yellow, ghosts per color)
                } else {
                    vram[0xFF0 + i*2] = 0;
                    vram[0xFF1 + i*2] = 0;
                }
            }
            
            // Set sprite positions in I/O ports
            // Pacman centered slightly lower
            memory_set_input_port(0x60, 112);  // Pacman X
            memory_set_input_port(0x61, 180);  // Pacman Y
            
            // Ghosts in cardinal positions around Pacman
            memory_set_input_port(0x62, 80);   // Ghost 1 X (left)
            memory_set_input_port(0x63, 180);  // Ghost 1 Y
            memory_set_input_port(0x64, 144);  // Ghost 2 X (right)
            memory_set_input_port(0x65, 180);  // Ghost 2 Y
            memory_set_input_port(0x66, 112);  // Ghost 3 X (up)
            memory_set_input_port(0x67, 140);  // Ghost 3 Y
            memory_set_input_port(0x68, 112);  // Ghost 4 X (down)
            memory_set_input_port(0x69, 220);  // Ghost 4 Y
            
            debug_log("Pacman demo screen created");
        }
        
        // Move sprites a bit each frame for animation
        static int frame_counter = 0;
        frame_counter++;
        
        if (frame_counter % 5 == 0) {  // Every 5 frames
            // Make Pacman move in a circle
            int pacman_x = io_read_byte(0x60);
            int pacman_y = io_read_byte(0x61);
            
            // Calculate new position in a circular path
            double angle = (frame_counter % 120) * 3.14159 * 2.0 / 120.0;
            pacman_x = 112 + (int)(30.0 * cos(angle));
            pacman_y = 180 + (int)(30.0 * sin(angle));
            
            memory_set_input_port(0x60, pacman_x);
            memory_set_input_port(0x61, pacman_y);
            
            // Move ghosts slightly to create movement
            for (int i = 1; i < 4; i++) {
                int ghost_x = io_read_byte(0x60 + i*2);
                int ghost_y = io_read_byte(0x61 + i*2);
                
                // Each ghost has a different movement pattern
                double ghost_angle = (frame_counter % 120) * 3.14159 * 2.0 / 120.0 + i * 3.14159 / 2.0;
                ghost_x = 112 + (int)(50.0 * cos(ghost_angle));
                ghost_y = 180 + (int)(50.0 * sin(ghost_angle));
                
                memory_set_input_port(0x60 + i*2, ghost_x);
                memory_set_input_port(0x61 + i*2, ghost_y);
            }
        }
    }
    
    // Skip CPU execution completely for now
    // This is a temporary measure until we can properly integrate Z80 emulation
    cycles_this_frame = CYCLES_PER_FRAME;
    
    // Handle interrupts at the end of the frame
    if (regs.iff1) {
        // Generate interrupt at VBLANK (end of frame)
        cpu_interrupt();
        handle_interrupt();
    }
    
    regs.cycles += cycles_this_frame;
}