C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o  cpu/gdt.o cpu/process.o boot/kernel_entry.o boot/bootsect.o }

# Change this if your cross-compiler is somewhere else
CC = gcc-7
GDB = gdb
# -g: Use debugging symbols in gcc
CFLAGS = -g -mgeneral-regs-only -mno-red-zone -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables

all: clean run

# First rule is run by default
os-image.bin: kernel.bin
	cat kernel.bin formatle.bin sym.bin str.bin > os-image.bin

# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
#kernel.bin: boot/bootsect.o boot/kernel_entry.o  ${OBJ}
#	ld -o $@ -Tlink.ld $^ -m elf_i386
#	objcopy -O binary --only-section=.text --only-section=.data --only-section=.bss kernel.bin text.bin
#	objcopy -O binary --only-section=.boot kernel.bin boot.bin
#	cat boot.bin text.bin > os-image.bin

kernel.bin:  ${OBJ}
	ld -o $@ -Tlink.ld $^ --oformat binary -m elf_i386
	ld -o kernel.elf -Tlink.ld $^ -m elf_i386
	# Calling the script that will create the symtable and dump it to binary. Also updating the
	# ld script, thus making us need to relink.
	./create_symtable.sh
	rm $@
	ld -o $@ -Tlink.ld $^ --oformat binary -m elf_i386

# Used for debugging purposes
kernel.elf: ${OBJ}
	ld -o $@ -Tlink.ld $^ -m elf_i386

run: os-image.bin
	qemu-system-i386 -fda os-image.bin

# Open the connection to qemu and load our kernel-object file with symbols
debug: os-image.bin kernel.elf
	qemu-system-i386 -s -fda os-image.bin &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# Generic rules for wildcards
# To make an object, always compile from its .c
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -fno-pie -ffreestanding -m32 -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@


clean:
	rm -rf *.bin *.dis *.o os-image.bin str.bin sym.bin *.elf
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o
