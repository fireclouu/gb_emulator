#include <stdio.h>
#include "includes/display.h"
#include "includes/host.h"
#include "includes/utils.h"
#include "gameboy/cpu.c"

int init(const char* fname) 
{
	printf("PROGRAM START\n\n");
    
    // Memory
    allocateMemory();
	if (loadFile(fname, 0) != 0) return 1;
	printf("Program size: %lu (%lu bits)\n", memory_size, 8 * memory_size);
	
    cpu = &sharp;
    cpu_regs_init(cpu);
    cpu->registers.pc = 0x0000;

    // Host Display 
    /*
    if (initWin() != 0) return 1;

    halt = 0;
    */
    return 0;
}

// pseudo-breakpoint for debugging purposes
uint8_t a, b, c;
size_t loopcount = 0;

int detectLockup() {
    switch(cpu->registers.pc) {
        case 0x64:
            a++;
            break;
        case 0x66:
            b++;
            break;
        case 0x68:
            c++;
            break;
        default:
            a = b = c = 0;
    }

    if ((a & b & c) == 3) {
        printf("ERROR! system locked up! loop cycle: %lu\n", loopcount);
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char** argv)
{
    if(init("roms/dmg_boot.bin") != 0) return 1;

    while(!halt) {
        printTrace(PRINT_FULL);
		cpu_exec(cpu);
        loopcount++;
        // setDisplay(memory);
        if (detectLockup()) return 1;

        if (cpu->registers.pc == 0xfe) {
            printf("Success!");
            break;
        }
	}

    closeWin();
	printf("\nPROGRAM END\n\n");

    return 0;
}
