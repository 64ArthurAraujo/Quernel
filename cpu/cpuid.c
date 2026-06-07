#include "include/cpuid.h"
#include "../drivers/include/screen.h"
#include "../libc/include/memory.h"
#include <stdint.h>

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

int cpuid_supported()
{
    uint32_t a, b;
    asm volatile(
        "pushfl\n"
        "pushfl\n"
        "popl %0\n"
        "movl %0, %1\n"
        "xorl $0x200000, %0\n"
        "pushl %0\n"
        "popfl\n"
        "pushfl\n"
        "popl %0\n"
        "popfl\n" : "=r"(a),
                    "=r"(b));
    return (a != b);
}

void cpuid_detect()
{
    if (!cpuid_supported())
    {
        kprintln("CPUID is not supported");
        return;
    }

    cpu_info_t cpu_info;

    uint32_t eax, ebx, ecx, edx;

    cpuid(0x0, &eax, &ebx, &ecx, &edx);

    mem_copy((uint8_t *)&ebx, (uint8_t *)&cpu_info.vendor[0], 4);
    mem_copy((uint8_t *)&edx, (uint8_t *)&cpu_info.vendor[4], 4);
    mem_copy((uint8_t *)&ecx, (uint8_t *)&cpu_info.vendor[8], 4);

    cpu_info.vendor[12] = '\0';

    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);

    if (eax >= 0x80000004)
    {
        uint32_t *brand_ptr = (uint32_t *) cpu_info.brand; 

        cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
        brand_ptr[0] = eax; brand_ptr[1] = ebx; brand_ptr[2] = ecx; brand_ptr[3] = edx;

        cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
        brand_ptr[4] = eax; brand_ptr[5] = ebx; brand_ptr[6] = ecx; brand_ptr[7] = edx;

        cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
        brand_ptr[8] = eax; brand_ptr[9] = ebx; brand_ptr[10] = ecx; brand_ptr[11] = edx;

        cpu_info.brand[48] = '\0';
    }

    kprint("CPU: ");
    kprint(cpu_info.vendor);
    kprint(" ");
    kprintln(cpu_info.brand);
}