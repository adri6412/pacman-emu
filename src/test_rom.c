#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ROM_SIZE 0x4000 // 16KB

// Use simple numbers 0-16 instead of ASCII for our test
// These will map to pre-defined patterns in our character set
#define H 0x01
#define E 0x02
#define L 0x03
#define O 0x04
#define W 0x05
#define R 0x06
#define D 0x07
#define SPACE 0x00

// Simple Z80 program for testing the emulator - draws "HELLO WORLD"
static const uint8_t test_program[] = {
    0xF3,       // DI (Disable interrupts)
    
    // Write "HELLO WORLD" to VRAM starting at position (10,10)
    // Calculate VRAM address: base 0x5000 + (y * 32) + x
    // For y=10, x=10: 0x5000 + (10 * 32) + 10 = 0x5000 + 0x140 + 0xA = 0x514A
    
    // H
    0x3E, H,    // LD A, 'H'
    0x32, 0x4A, 0x51, // LD (0x514A), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4A, 0x55, // LD (0x554A), A - corresponding color RAM
    
    // E 
    0x3E, E,    // LD A, 'E'
    0x32, 0x4B, 0x51, // LD (0x514B), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4B, 0x55, // LD (0x554B), A
    
    // L
    0x3E, L,    // LD A, 'L'
    0x32, 0x4C, 0x51, // LD (0x514C), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4C, 0x55, // LD (0x554C), A
    
    // L
    0x3E, L,    // LD A, 'L'
    0x32, 0x4D, 0x51, // LD (0x514D), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4D, 0x55, // LD (0x554D), A
    
    // O
    0x3E, O,    // LD A, 'O'
    0x32, 0x4E, 0x51, // LD (0x514E), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4E, 0x55, // LD (0x554E), A
    
    // Space
    0x3E, SPACE,// LD A, ' '
    0x32, 0x4F, 0x51, // LD (0x514F), A
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x4F, 0x55, // LD (0x554F), A
    
    // W
    0x3E, W,    // LD A, 'W'
    0x32, 0x50, 0x51, // LD (0x5150), A
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x50, 0x55, // LD (0x5550), A
    
    // O
    0x3E, O,    // LD A, 'O'
    0x32, 0x51, 0x51, // LD (0x5151), A
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x51, 0x55, // LD (0x5551), A
    
    // R
    0x3E, R,    // LD A, 'R'
    0x32, 0x52, 0x51, // LD (0x5152), A
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x52, 0x55, // LD (0x5552), A
    
    // L
    0x3E, L,    // LD A, 'L'
    0x32, 0x53, 0x51, // LD (0x5153), A
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x53, 0x55, // LD (0x5553), A
    
    // D
    0x3E, D,    // LD A, 'D'
    0x32, 0x54, 0x51, // LD (0x5154), A
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x54, 0x55, // LD (0x5554), A
    
    0xFB,       // EI (Enable interrupts)
    0x76,       // HALT
    
    // Interrupt handler at 0x0038
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    0x3C,       // INC A (0x0038 - Interrupt vector)
    0xC9,       // RET
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <output_file>\n", argv[0]);
        return 1;
    }
    
    // Create a ROM file with the test program
    uint8_t *rom = (uint8_t *)malloc(ROM_SIZE);
    if (!rom) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Fill ROM with RST 38h instructions (0xFF)
    memset(rom, 0xFF, ROM_SIZE);
    
    // Copy test program to ROM
    memcpy(rom, test_program, sizeof(test_program));
    
    // Write ROM to file
    FILE *file = fopen(argv[1], "wb");
    if (!file) {
        fprintf(stderr, "Failed to create output file: %s\n", argv[1]);
        free(rom);
        return 1;
    }
    
    size_t written = fwrite(rom, 1, ROM_SIZE, file);
    if (written != ROM_SIZE) {
        fprintf(stderr, "Failed to write ROM data: %zu of %d bytes written\n", written, ROM_SIZE);
    } else {
        printf("Created test ROM: %s (%d bytes)\n", argv[1], ROM_SIZE);
    }
    
    fclose(file);
    free(rom);
    
    return 0;
}