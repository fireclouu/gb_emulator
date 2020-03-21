#include <stdio.h>
#include "includes/display.h"
#include "includes/host.h"
#include "shared.h"
#include "includes/utils.h"
#include "gameboy/cpu.c"

int init(const char* fname) 
{
	printf("PROGRAM START\n\n");

    // reset cycle counters
    cycle_bytes = cycle_cpu = 0;

    // Memory
    allocateMemory();
	if (loadFile(fname, 0) != 0) return 1;
	printf("Program size: %lu (%lu bits)\n", memory_size, 8 * memory_size);
	
    cpu = &sharp;
    cpu_regs_init(cpu);
    cpu->registers.pc = 0x0000;

    // Host Display 
    //if (initWin() != 0) return 1;

    halt = 0;

    return 0;
}

// pseudo-breakpoint for debugging purposes
uint8_t a, b, c;
size_t loopcount = 0;

int detectLockup() {
    switch(memory[cpu->registers.pc]) {
        case 0x00:
            a++;
            break;
        default:
            a = b = c = 0;
    }

    if ((a) == 10) {
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
        // time
        gettimeofday(&tv, NULL);
        
        // unify time on next executions
        timer = USYSTIME;
        
        //print
        printTrace(PRINT_FULL);

        /*if (cpu->registers.pc == 0x00) { // disable
            printf("%04x reached", cpu->registers.pc);
            return 0;
        }*/

        // service interrupt
        if ((memory[ADDR_IF] & 0x1f) && !(memory[ADDR_IE]))
        {
            // by priority
            if (memory[ADDR_IF] & 0x1)
            {
                memory[ADDR_IF] &= 0xfe;
                memory[ADDR_IE] |= 0x1;
                cpu_inst_rst(cpu, 0x40);
            }
        }
        // cpu
		cpu_exec(cpu);

        // polling
        gpu_int_ly(0);

        //loopcount++;
        //setDisplay(memory);
        //if (detectLockup()) return 1;
	}

    closeWin();
	printf("\nPROGRAM END\n\n");

    return 0;
}
