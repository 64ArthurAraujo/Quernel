CC  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-gcc
LD  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-ld
CFLAGS = -g -m32 -ffreestanding

ISO_DIR = iso/boot/grub

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
HEADERS   = $(wildcard */include/*.h)
OBJ       = $(C_SOURCES:.c=.o) cpu/interrupt.o

kernel.elf: boot/kernel_entry.o $(OBJ)
	$(LD) -T linker.ld -o $@ $^

quernel.iso: kernel.elf
	mkdir -p $(ISO_DIR)
	cp kernel.elf iso/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/grub.cfg
	grub2-mkrescue -o quernel.iso iso/

run: quernel.iso
	qemu-system-i386 -cdrom quernel.iso

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

clean:
	rm -f $(OBJ) kernel.elf quernel.iso boot/kernel_entry.o
	rm -r ./iso

