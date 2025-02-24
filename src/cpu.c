#include "../include/cpu.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    // Pacman Z80 runs at 3.072 MHz, so one frame is about 50,000 cycles
    const uint32_t CYCLES_PER_FRAME = 50000;
    
    regs.cycles = 0;
    
    // Process any pending interrupt
    if (interrupt_pending) {
        handle_interrupt();
    }
    
    // Execute instructions until we've run enough cycles for one frame
    while (regs.cycles < CYCLES_PER_FRAME) {
        // If CPU is halted, just count cycles and wait for interrupt
        if (regs.halted) {
            regs.cycles += 4;
            continue;
        }
        
        // Fetch opcode
        uint8_t opcode = fetch_byte();
        
        // For a real Z80 emulator, we would have a large function to handle all opcodes
        // For this simplified version, we'll just track cycles and increment PC
        
        // Count cycles for this instruction
        regs.cycles += cycle_counts[opcode];
        
        // Here we would execute the instruction based on opcode
        // This is where you would implement the full Z80 instruction set
        
        // For now, we just simulate some basic behavior
        switch (opcode) {
            case 0x00: // NOP
                break;
                
            case 0x76: // HALT
                regs.halted = true;
                break;
                
            case 0xFB: // EI
                regs.iff1 = true;
                regs.iff2 = true;
                break;
                
            case 0xF3: // DI
                regs.iff1 = false;
                regs.iff2 = false;
                break;
                
            // Add more instructions as needed...
                
            default:
                // For unimplemented instructions, just simulate them
                if (opcode >= 0xC0) {  // Call and return instructions
                    if ((opcode & 0x0F) == 0x09) { // RET
                        regs.pc = pop();
                    } else if ((opcode & 0x0F) == 0x0D) { // CALL
                        uint16_t addr = fetch_word();
                        push(regs.pc);
                        regs.pc = addr;
                    }
                }
                break;
        }
    }
}