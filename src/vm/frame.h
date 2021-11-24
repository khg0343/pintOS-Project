#ifndef VM_FRAME_H
#define VM_FRAME_H
struct lock lru_lock;
void lru_list_init();
void add_page_to_lru_list(struct page* page);
void del_page_from_lru_list(struct page* page);


struct page* find_page_lru(void *kaddr);
struct page* isVictim();

#endif