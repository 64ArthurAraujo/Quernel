#pragma once

typedef struct {
    char vendor[13];
    char brand[49];
} cpu_info_t;

void cpuid_detect();