#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t data;

    asm volatile("inb %w1,%0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void outb(uint8_t value, uint16_t port) {
    asm volatile("outb %b0,%w1" : : "a" (value), "Nd" (port));
}