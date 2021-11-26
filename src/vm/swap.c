#include "vm/swap.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"

struct lock lock_swap;
struct block *block_swap;
struct bitmap *bitmap_swap;

void swap_init(size_t size)
{
    lock_init(&lock_swap);
    bitmap_swap = bitmap_create(size);
    block_swap = block_get_role(BLOCK_SWAP);
    if (!block_swap)
        return;

    bitmap_swap = bitmap_create(block_size(block_swap) / 8);
    if (!bitmap_swap)
        return;
}

void swap_in(size_t used_index, void *kaddr)
{
    block_swap = block_get_role(BLOCK_SWAP);
    if (used_index-- == 0)
        NOT_REACHED();

    lock_acquire(&lock_file);
    lock_acquire(&lock_swap);

    int i;
    for (i = 0; i < 8; i++)
        block_read(block_swap, used_index * 8 + i, kaddr + BLOCK_SECTOR_SIZE * i);

    bitmap_set_multiple(bitmap_swap, used_index, 1, false);
    lock_release(&lock_swap);
    lock_release(&lock_file);
}

void swap_free(size_t used_index)
{
    if (used_index-- == 0)
        return;
    lock_acquire(&lock_swap);
    bitmap_set_multiple(bitmap_swap, used_index, 1, false);
    lock_release(&lock_swap);
}

size_t swap_out(void *kaddr)
{
    block_swap = block_get_role(BLOCK_SWAP);
    lock_acquire(&lock_file);
    lock_acquire(&lock_swap);
    size_t index_swap = bitmap_scan_and_flip(bitmap_swap, 0, 1, false);
    if (BITMAP_ERROR == index_swap)
    {
        NOT_REACHED();
        return BITMAP_ERROR;
    }

    int i;
    for (i = 0; i < 8; i++)
        block_write(block_swap, index_swap * 8 + i, kaddr + BLOCK_SECTOR_SIZE * i);

    lock_release(&lock_swap);
    lock_release(&lock_file);
    return index_swap + 1;
}