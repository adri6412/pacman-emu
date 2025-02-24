#include "../include/cpu.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>  // For SDL_Delay

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
    static bool executed_rom = false;
    
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
    
    // Pacman Z80 runs at 3.072 MHz, so one frame is about 50,000 cycles
    const uint32_t CYCLES_PER_FRAME = 50000;
    
    // Simulate Z80 execution but with safety guards to prevent infinite loops
    uint32_t cycles_this_frame = 0;
    uint32_t max_instructions = 10000; // Safety limit to prevent infinite loops
    uint32_t instruction_count = 0;
    
    while (cycles_this_frame < CYCLES_PER_FRAME && instruction_count < max_instructions) {
        // SAFETY CHECK: Ensure PC is within valid ROM/RAM bounds
        if (regs.pc >= 0x8000) {
            debug_log("ERROR: PC out of bounds: 0x%04X - resetting to 0", regs.pc);
            regs.pc = 0;  // Reset to ROM start
            regs.halted = true;
            break;
        }
        
        if (regs.halted) {
            // If CPU is halted, just count cycles until an interrupt
            cycles_this_frame += 4;
            continue;
        }
        
        // Fetch the current opcode
        uint8_t opcode = cpu_read_byte(regs.pc);
        uint8_t cycles = cycle_counts[opcode];
        
        // Debug output for instructions
        if (instruction_count < 20 || opcode != 0xFF) {
            debug_log("Executing opcode %02X at PC=%04X", opcode, regs.pc);
        }
        
        // Handle Z80 instructions
        switch (opcode) {
            case 0x00:  // NOP
                regs.pc++;
                break;
                
            case 0x3E:  // LD A, n
                regs.a = fetch_byte();
                regs.pc++;
                break;
                
            case 0x47:  // LD B, A
                regs.b = regs.a;
                regs.pc++;
                break;
                
            case 0x76:  // HALT
                regs.halted = true;
                regs.pc++;
                break;
                
            case 0xC3:  // JP nn
                {
                    uint16_t jump_addr = fetch_word();
                    // SAFETY CHECK: Avoid jumping outside ROM
                    if (jump_addr < 0x8000) {
                        regs.pc = jump_addr;
                    } else {
                        debug_log("ERROR: Jump to invalid address: 0x%04X", jump_addr);
                        regs.pc = 0;  // Reset to start
                        regs.halted = true;
                    }
                }
                break;
                
            case 0xCD:  // CALL nn
                {
                    uint16_t call_addr = fetch_word();
                    // SAFETY CHECK: Avoid calling outside ROM
                    if (call_addr < 0x8000) {
                        push(regs.pc);
                        regs.pc = call_addr;
                    } else {
                        debug_log("ERROR: Call to invalid address: 0x%04X", call_addr);
                        regs.pc += 2;  // Skip the address bytes
                    }
                }
                break;
                
            case 0xC9:  // RET
                regs.pc = pop();
                break;
                
            case 0xED:  // Extended instructions
                {
                    uint8_t extended_op = fetch_byte();
                    if (extended_op == 0x47) {  // LD I, A
                        regs.i = regs.a;
                    } else {
                        debug_log("Unhandled extended opcode: ED %02X", extended_op);
                    }
                    regs.pc++;
                }
                break;
                
            case 0xF3:  // DI
                regs.iff1 = false;
                regs.iff2 = false;
                regs.pc++;
                break;
                
            case 0xFB:  // EI
                regs.iff1 = true;
                regs.iff2 = true;
                regs.pc++;
                break;
                
            case 0xFF:  // RST 38h
                // This is a common "filler" in ROM, handle it gracefully
                push(regs.pc + 1);
                regs.pc = 0x0038;  // Jump to interrupt vector
                break;
                
            default:
                // For unimplemented opcodes, just advance PC
                debug_log("Unimplemented opcode: %02X at PC=%04X", opcode, regs.pc);
                regs.pc++;
                break;
        }
        
        cycles_this_frame += cycles;
        instruction_count++;
        
        // If we've executed more than 100 instructions, insert a small delay 
        // to prevent GUI freezing (this is a temporary fix)
        if (instruction_count >= 100 && instruction_count % 100 == 0) {
            // Yield to the OS to avoid freezing the GUI
            SDL_Delay(1);
        }
    }
    
    if (instruction_count >= max_instructions) {
        debug_log("WARNING: Hit instruction limit (%d) - possible infinite loop", max_instructions);
    }
    
    // Handle interrupts at the end of the frame
    if (regs.iff1) {
        // Generate interrupt at VBLANK (end of frame)
        cpu_interrupt();
        handle_interrupt();
    }
    
    regs.cycles += cycles_this_frame;
}