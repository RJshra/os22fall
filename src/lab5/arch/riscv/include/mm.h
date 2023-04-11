#include "types.h"
#include "stdint.h"
struct run {
    struct run *next;
};

void mm_init();

uint64 kalloc();
void kfree(uint64);

struct buddy {
  uint64_t size;
  uint64_t *bitmap; 
};

void buddy_init();
uint64_t  buddy_alloc(uint64_t);
void buddy_free(uint64_t);

uint64_t alloc_pages(uint64_t);
uint64_t alloc_page();
void free_pages(uint64_t);
