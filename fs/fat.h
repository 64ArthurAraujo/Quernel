#pragma once
#include <stdint.h>

struct __attribute__((packed)) fat12_bpb
{
    uint8_t jmp[3];
}