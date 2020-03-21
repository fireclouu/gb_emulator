#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#define USYSTIME tv.tv_sec*1000000+tv.tv_usec
#define ADDR_IE 0xffff
#define ADDR_IF 0xff0f

uint8_t *memory;
bool halt;
size_t usystime;

// time
struct timeval tv;
size_t timer;

// cycles
size_t cycle_cpu;
size_t cycle_bytes;

#endif // header guard
