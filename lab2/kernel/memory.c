#include "memory.h"

#include "lib/stdlib.h"

extern int __bss_end;
#define HEAP_MAX (&__bss_end) + 0x100

static char *heap_top;

void init_memory() {
    heap_top = (char *)HEAP_MAX;
    heap_top++;
}

void *simple_malloc(unsigned int size) {
    if (size == 0) {
        return NULL;
    }
    void *ret = heap_top;
    heap_top += size;
    return ret;
}

char *get_heap_top() { return heap_top; }