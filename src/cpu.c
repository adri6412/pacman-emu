#include "../include/cpu.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declaration of debug_log function
extern void debug_log(const char *format, ...);

// CPU state
static Z80_Registers regs;
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

// CPU initialization (based on MAME Z80 implementation)
void cpu_init(void) {
    cpu_reset();
}

// Reset the CPU to initial state (based on MAME Z80 implementation)
void cpu_reset(void) {
    // Clear all registers
    memset(&regs, 0, sizeof(regs));
    
    // Standard Z80 reset state
    regs.pc = 0;            // Start at address 0 (ROM)
    regs.sp = 0xF000;       // Initial stack pointer in high RAM
    regs.iff1 = false;      // Interrupts disabled
    regs.iff2 = false;
    regs.im = 0;            // Interrupt mode 0
    regs.i = 0;             // Interrupt register
    regs.r = 0;             // Refresh register
    regs.halted = false;    // Not halted
    regs.cycles = 0;
    
    // Initialize flags to known values (some games check for specific flag bits)
    regs.f = FLAG_F3 | FLAG_F5; // Set undocumented flags
    
    interrupt_pending = false;
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
    if (regs.iff1) {
        interrupt_pending = true;
    }
}

// Process a pending interrupt
static void handle_interrupt(void) {
    if (interrupt_pending && regs.iff1) {
        interrupt_pending = false;
        regs.iff1 = false;
        regs.iff2 = false;
        
        if (regs.halted) {
            regs.halted = false;
            regs.pc++;
        }
        
        switch (regs.im) {
            case 0:
                // Mode 0: Execute instruction on data bus (0xFF for Pacman)
                // In this simplified emulation, we just handle RST 38h
                push(regs.pc);
                regs.pc = 0x0038;
                regs.cycles += 11;
                break;
                
            case 1:
                // Mode 1: Jump to 0x0038
                push(regs.pc);
                regs.pc = 0x0038;
                regs.cycles += 13;
                break;
                
            case 2:
                // Mode 2: Use I register and data bus (0xFF for Pacman)
                {
                    uint16_t addr = (regs.i << 8) | 0xFF;
                    uint16_t jump_addr = memory_read_word(addr);
                    push(regs.pc);
                    regs.pc = jump_addr;
                    regs.cycles += 19;
                }
                break;
        }
    }
}

// Execute CPU instructions for one frame (~16.6ms)
// For simplicity, we'll run a fixed number of cycles per frame
void cpu_execute_frame(void) {
    // For testing purposes, we'll create some pattern in VRAM directly
    // since we don't have a full CPU emulation yet
    static bool initialized_vram = false;
    
    if (!initialized_vram) {
        debug_log("Initializing test pattern in VRAM");
        
        // Get VRAM and CRAM memory
        uint8_t *vram = memory_get_vram();
        uint8_t *cram = memory_get_cram();
        
        if (vram && cram) {
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
            
            // Add Pacman and ghosts to specific positions
            int pacman_pos = 15 * 32 + 14;
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
    
    // Pacman Z80 runs at 3.072 MHz, so one frame is about 50,000 cycles
    const uint32_t CYCLES_PER_FRAME = 50000;
    
    // Just count cycles for now - we're not executing actual instructions
    regs.cycles += CYCLES_PER_FRAME;
}