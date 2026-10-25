CC  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-gcc
LD  = /home/arthur/Downloads/i686-elf-tools-linux/bin/i686-elf-ld
CFLAGS = -g -m32 -ffreestanding -Iinclude -Wmaybe-uninitialized 

ISO_DIR = iso/boot/grub

# root dir of the os
DATA_DIR     = rootfs
DATA_IMG     = data.ext2
DATA_SIZE_MB = 256

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c fs/*.c kernel/printk/*.c fs/*/*.c)
HEADERS   = $(wildcard */*.h */*/*.h)
OBJ       = $(C_SOURCES:.c=.o) cpu/interrupt.o

DATA_FILES = $(shell find $(DATA_DIR) 2>/dev/null)

debug: quernel.iso kernel.elf
	qemu-system-i386 -S -s -drive format=raw,file=quernel.iso,index=0,media=disk &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

kernel.elf: boot/kernel_entry.o $(OBJ)
	@echo "  LD      $@"
	@$(LD) -T linker.ld -o $@ $^

$(DATA_IMG): $(DATA_FILES)
	@echo "  EXT2    $@"
	@rm -f $@
	@mke2fs -q -t ext2 -F -d $(DATA_DIR) $@ $(DATA_SIZE_MB)M

quernel.iso: kernel.elf $(DATA_IMG)
	@echo "  ISO     $@"
	@mkdir -p $(ISO_DIR)
	@cp kernel.elf iso/boot/kernel.elf
	@cp grub.cfg $(ISO_DIR)/grub.cfg
	@grub2-mkrescue -o quernel.iso iso/ 2>/dev/null
	@echo "  PART    $@ (+ext2 $(DATA_SIZE_MB)M)"
	@set -e; \
	ISO_SIZE=$$(stat -c %s quernel.iso); \
	START=$$(( (ISO_SIZE + 1048575) / 1048576 )); \
	truncate -s $$(( (START + $(DATA_SIZE_MB) + 1) * 1048576 )) quernel.iso; \
	dd if=$(DATA_IMG) of=quernel.iso bs=1M seek=$$START conv=notrunc 2>/dev/null; \
	FIRST=$$(( START * 2048 )); \
	LAST=$$(( FIRST + $(DATA_SIZE_MB) * 2048 - 1 )); \
	sgdisk -e quernel.iso >/dev/null; \
	sgdisk -n 0:$$FIRST:$$LAST -t 0:8300 -c 0:data quernel.iso >/dev/null

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
	@rm -f $(OBJ) kernel.elf quernel.iso boot/kernel_entry.o $(DATA_IMG)
	@rm -rf ./iso