#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ROM_SIZE 0x4000 // 16KB

// Simple Z80 program for testing the emulator
static const uint8_t test_program[] = {
    0xF3,       // DI (Disable interrupts)
    0x3E, 0x34, // LD A, 0x34
    0x32, 0x00, 0x50, // LD (0x5000), A - Write to first byte of VRAM
    0x3E, 0x06, // LD A, 0x06 (yellow color)
    0x32, 0x00, 0x54, // LD (0x5400), A - Write to first byte of CRAM
    0x3E, 0x01, // LD A, 0x01 (blue color)
    0x32, 0x01, 0x54, // LD (0x5401), A - Write to second byte of CRAM
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