#include "timer.h"
#include "isr.h"
#include "../drivers/tty.h"
#include "../cpu/ports.h"
#include "../libc/string.h"
#include <stdint.h>

uint32_t tick = 0;

static void timer_callback(registers_t regs)
{
    tick++;
    tty_emit("Tick: ");

    char tick_ascii[256];

    int_to_ascii(tick, tick_ascii);

    tty_emit(tick_ascii);
    tty_emit("\n");
}

void init_timer(uint32_t frequency)
{
    // set the timer callback function
    register_interrupt_handler(IRQ0, timer_callback); // IRQ0 receives timer_callback();

    uint32_t divisor = 1193180 / frequency;
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    port_byte_out(0x43, 0x36); // command port
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}