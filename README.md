# Quernel
The Quernel is a 32-bit (at least for now) kernel that i develop in the free time for fun,
its name is a play with the portuguese pronunciation of the word "Kernel", that if we would write 
exactly like we pronounce would be spelled as "Quernel". The final goal is to have it 
running as a UNIX-like OS, being able to run something like X11 and maybe a full desktop like GNOME.

## Running
To be able to build and run the Quernel you will need three things: GCC and GDB, An cross compiler, and QEMU.
The cross compiler can be downloaded from https://github.com/lordmilko/i686-elf-tools/releases/tag/15.2.0 and the rest
you can figure it out by your own. After getting all the items installed, extract the cross compiler and replace the 
`CC` and `LD` from the makefile with the path to your downloaded i686-elf-tools

With that being done you can now run the Quernel using the command `make run`. Run generates a ISO file that is the root
disk of the quernel, and that can also be flashed in a usb stick and run on bare metal later on (be careful when doing it).

### showcases.c
In the kernel folder we have the file showcases.c, as the description says it contains a list of scenarios to play
with the quernel you can try switching the default `read_hello_world()` for the `read_hello_world_broken()` to see 
how the kernel will behave and report that something is broken.

## Credits
Linux - For almost anything new i try to implement.

[MyraOS](https://github.com/dvir-biton/MyraOS) - For the kernel memory allocation implementations
