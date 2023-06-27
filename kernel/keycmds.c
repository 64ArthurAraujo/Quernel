#include "../libc/include/string.h"
#include "../drivers/include/screen.h"
#include "../libc/include/memory.h"
#include <stdint.h>

void user_input(char *input)
{
    if (strcmp(input, "HELP") == 0)
    {
        kprint("*** Possible kernel commands *** \n");
        kprint("PAGE - Calls the kmalloc() function. \n");
        kprint("END - Halts the CPU. \n");
    }
    else if (strcmp(input, "PAGE") == 0)
    {
        uint32_t physical_addr;
        uint32_t page = kmalloc(1000, 1, &physical_addr);

        char page_str[16] = "";
        hex_to_ascii(page, page_str);

        char physical_addr_str[16] = "";
        hex_to_ascii(physical_addr, physical_addr_str);

        kprint("PAGE: ");
        kprint(page_str);
        kprint("\n");

        kprint("ADDRESS: ");
        kprint(physical_addr_str);
        kprint("\n");
    }
    else if (strcmp(input, "END") == 0)
    {
        kprint("Stopping the CPU... \n");
        kprint("Bye!");

        asm volatile("hlt");
    }
    else
    {
        kprint("'");
        kprint(input);
        kprint("' is not a recognized instruction for the kernel. \n");
    }
}