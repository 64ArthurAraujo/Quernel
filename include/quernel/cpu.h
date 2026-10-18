#pragma once
#include <stdint.h>

// cpuid.c
void cpuid_detect();

// isr.c
void isr_setup();

// ports.h
unsigned char port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);
unsigned short port_word_in(unsigned short port);
void port_word_out(unsigned short port, unsigned short data);

// timer.c
void init_timer(uint32_t freq);