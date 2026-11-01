#include <drivers/tty.h>
#include <string.h>
#include <stdint.h>

#include "timer.h"
#include "isr.h"
#include "ports.h"

volatile uint32_t tick = 0;
static uint32_t timer_freq = 0;

uint32_t timer_get_ticks(void)
{
    return tick;
}

static void timer_callback(registers_t regs)
{
    tick++;
}

void sleep(uint32_t ms)
{
    if (timer_freq == 0)
        return;

    uint32_t ticks = (ms / 1000) * timer_freq + ((ms % 1000) * timer_freq + 999) / 1000;

    uint32_t start = tick;

    while ((uint32_t)(tick - start) < ticks)
        asm volatile("hlt");
}

void init_timer(uint32_t frequency)
{
    timer_freq = frequency;
    register_interrupt_handler(IRQ0, timer_callback);

    uint32_t divisor = 1193180 / frequency;
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    port_byte_out(0x43, 0x36);
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}