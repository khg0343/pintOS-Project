#ifndef VM_FRAME_H
#define VM_FRAME_H
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"

void lru_list_init(void);
void add_page_to_lru_list(struct page *page);
void del_page_from_lru_list(struct page *page);
struct page* find_page_in_lru_list(void *kaddr);
void* try_to_free_pages(enum palloc_flags flags);

#endif