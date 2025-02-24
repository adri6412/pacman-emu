#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

// Z80 CPU registers
typedef struct {
    union {
        struct {
            uint8_t f;
            uint8_t a;
        };
        uint16_t af;
    };
    
    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };
    
    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };
    
    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };
    
    uint16_t ix;
    uint16_t iy;
    uint16_t sp;
    uint16_t pc;
    
    // Alternate registers
    uint16_t af_prime;
    uint16_t bc_prime;
    uint16_t de_prime;
    uint16_t hl_prime;
    
    // Interrupt registers
    bool iff1;
    bool iff2;
    uint8_t i;
    uint8_t r;
    
    // CPU state
    uint8_t im;         // Interrupt mode
    bool halted;
    uint32_t cycles;    // Cycle counter
} Z80_Registers;

// CPU Flag bits
#define FLAG_C  0x01  // Carry
#define FLAG_N  0x02  // Subtract
#define FLAG_PV 0x04  // Parity/Overflow
#define FLAG_H  0x10  // Half carry
#define FLAG_Z  0x40  // Zero
#define FLAG_S  0x80  // Sign

// Function prototypes
void cpu_init(void);
void cpu_reset(void);
void cpu_execute_frame(void);
uint8_t cpu_read_byte(uint16_t address);
void cpu_write_byte(uint16_t address, uint8_t value);
void cpu_interrupt(void);

#endif // CPU_H