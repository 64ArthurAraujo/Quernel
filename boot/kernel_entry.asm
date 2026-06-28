[bits 32]

MULTIBOOT_MAGIC     equ 0x1BADB002
MULTIBOOT_FLAGS     equ 0x00
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
global _start
_start: 
    mov esp, stack_top

    [extern init_kernel]
    call init_kernel
    
    jmp $
    
section .bss
align 16
stack_bottom:
    resb 65536          
stack_top:
