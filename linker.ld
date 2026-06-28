ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);   /* r-x: read + execute */
    data PT_LOAD FLAGS(6);   /* rw-: read + write   */
}

SECTIONS {
    . = 0x100000;

    .multiboot : { *(.multiboot) } :text
    .text      : { *(.text)      } :text
    .rodata    : { *(.rodata)    } :text

    .data : { *(.data)          } :data
    .bss  : { *(.bss) *(COMMON) } :data
}