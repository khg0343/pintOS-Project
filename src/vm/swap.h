#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <stddef.h>
#include "userprog/syscall.h"
extern struct lock lock_file;
void swap_init(size_t size);
void swap_in(size_t used_index, void* kaddr);
void swap_free(size_t used_index);
size_t swap_out(void* kaddr);
void swap_clear(size_t used_index);

#endif