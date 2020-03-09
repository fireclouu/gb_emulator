#ifndef _HOST_H
#define _HOST_H

#include <stdio.h>

size_t file_size;
size_t memory_size;

void allocateMemory();
int loadFile(const char*, size_t);
void printTrace(int);
int init(const char* fname);

#endif
