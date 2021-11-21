#ifndef VM_FRAME_H
#define VM_FRAME_H

void lru_list_init(void);
void add_page_to_lru_list(struct page* page);
void del_page_from_lru_list(struct page* page);
static struct list_elem* get_next_lru_clock();

void try_to_free_pages(enum palloc_flags flags);

#endif