CC = /home/arthur/ccompilers/bin/i686-elf-gcc
LD = /home/arthur/ccompilers/bin/i686-elf-ld
GDB = /home/arthur/ccompilers/bin/i686-elf-gdb
CFLAGS = -g 

C_SOURCES = $(wildcard */*.c)
HEADERS = $(wildcard */include/*.h)

OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o}

CLEAN_TARGETS = $(wildcard *.o *.bin *.elf */*.o */*.bin */*.elf)

os-image.bin: boot/bootsect.bin kernel.bin
	cat $^ > os-image.bin

kernel.bin: boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ --oformat binary 

kernel.elf: boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ 

add-files:	os-image.bin
	cat root/hello.q >> os-image.bin

run: add-files
	qemu-system-i386 -drive format=raw,file=os-image.bin

debug: os-image.bin kernel.elf
	qemu-system-i386 -s -fda os-image.bin &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -ffreestanding -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf ${CLEAN_TARGETS}

