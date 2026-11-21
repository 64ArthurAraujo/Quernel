#pragma once
#include <stdint.h>
#include <drivers/ata.h>
#include <fs/ext.h>

// cpuid.c

void cpuid_detect(ext2_t *fs);

// isr.c
void isr_setup();

// ports.h
unsigned char port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);
unsigned short port_word_in(unsigned short port);
void port_word_out(unsigned short port, unsigned short data);

// timer.c
void init_timer(uint32_t freq);
uint32_t timer_get_ticks(void);
void sleep(uint32_t ms);