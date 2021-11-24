#include "vm/swap.h"
#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "vm/page.h"

struct lock swap_lock;
struct bitmap *swap_bitmap;
struct block *swap_block;

void swap_init(size_t used_index, void *kaddr)
{
    swap_block = block_get_role(BLOCK_SWAP);
    if (!swap_block) return;

    swap_bitmap = bitmap_create(block_size(swap_block) / 8);
    if (!swap_bitmap) return;

    bitmap_set_all(swap_bitmap, false);

    lock_init(&swap_lock);
}
void swap_in(size_t used_index, void *kaddr)
{
    //used_index의 swap slot에 저장된 데이터를 논리 주소 kaddr로 복사
    if (!bitmap_test(swap_bitmap, used_index)) return;

    int i;
    for (i = 0; i < 8; i++)
        block_read(swap_block, used_index * 8 + i, kaddr + BLOCK_SECTOR_SIZE * i);

    bitmap_flip(swap_bitmap, used_index);
}
size_t swap_out(void *kaddr)
{
    //  kaddr 주소가 가리키는 페이지를 스왑 파티션에 기록
    //  페이지를 기록한 swap slot 번호를 리턴

    size_t swap_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);

    int i;
    for (i = 0; i < 8; i++)
        block_write(swap_block, swap_index * 8 + i, kaddr + BLOCK_SECTOR_SIZE * i);

    return swap_index + 1;
}