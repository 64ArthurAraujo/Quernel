#pragma once

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void init_timer(uint32_t freq);
uint32_t timer_get_ticks(void);
void sleep(uint32_t ms);


#endif