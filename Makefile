CC  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-gcc
LD  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-ld
CFLAGS = -g -m32 -ffreestanding

ISO_DIR = iso/boot/grub

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c fs/*.c kernel/printk/*.c fs/fat12/*.c)
HEADERS   = $(wildcard */*.h */*/*.h)
OBJ       = $(C_SOURCES:.c=.o) cpu/interrupt.o

debug: quernel.iso kernel.elf
	qemu-system-i386 -S -s -drive format=raw,file=quernel.iso,index=0,media=disk &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

kernel.elf: boot/kernel_entry.o $(OBJ)
	@echo "  LD      $@"
	@$(LD) -T linker.ld -o $@ $^

quernel.iso: kernel.elf
	@echo "  ISO     $@"
	@mkdir -p $(ISO_DIR)
	@cp kernel.elf iso/boot/kernel.elf
	@cp grub.cfg $(ISO_DIR)/grub.cfg
	@grub2-mkrescue -o quernel.iso iso/ 2>/dev/null

run: quernel.iso
	qemu-system-i386 -drive format=raw,file=quernel.iso,index=0,media=disk

iso: quernel.iso

build: kernel.elf

%.o: %.c $(HEADERS)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "  NASM    $<"
	@nasm $< -f elf -o $@

clean:
	@echo "  CLEAN"
	@rm -f $(OBJ) kernel.elf quernel.iso boot/kernel_entry.o
	@rm -rf ./iso