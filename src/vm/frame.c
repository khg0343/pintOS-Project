#include "vm/frame.h"
#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "vm/page.h"

struct list lru_list;
struct lock lru_list_lock;
struct list_elem *lru_clock;

static struct list_elem *get_next_lru_clock(void);

void lru_list_init(void)
{
    list_init(&lru_list);
    lock_init(&lru_list_lock);
    lru_clock = NULL;
}

void add_page_to_lru_list(struct page *page)
{
    list_push_back(&lru_list, &page->lru);
}

void del_page_from_lru_list(struct page *page)
{
    list_remove(&page->lru);
}

struct page *find_page_in_lru_list(void *kaddr)
{
    struct list_elem *e;
    for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e))
    {
        struct page *page = list_entry(e, struct page, lru);
        if (page->kaddr == kaddr)
            return page;
    }
}

static struct list_elem *get_next_lru_clock()
{
    /* Clock 알고리즘의 LRU리스트를 이동하는 작업 수행
       LRU리스트의 다음 노드의 위치를 반환
       현재 LRU리스트가 마지막 노드 일 때 NULL 값을 반환*/

    if (lru_clock == NULL || lru_clock == list_end(&lru_list))
    {
        if (list_empty(&lru_list)) return NULL;
        else return (lru_clock = list_begin(&lru_list));
    }
    lru_clock = list_next(lru_clock);
    if (lru_clock == list_end(&lru_list))
        return get_next_lru_clock();
    return lru_clock;
}

void* try_to_free_pages(enum palloc_flags flags)
{
    struct thread *page_thread;
    struct list_elem *element;
    struct page *lru_page;

    lock_acquire(&lru_list_lock);

    if (list_empty(&lru_list) == true)
    {
        lock_release(&lru_list_lock);
        return;
    }

    while (true)
    {
        element = get_next_lru_clock();
        if (element == NULL)
        {
            lock_release(&lru_list_lock);
            return;
        }

        lru_page = list_entry(element, struct page, lru);

        page_thread = lru_page->thread;
        if (pagedir_is_accessed(page_thread->pagedir, lru_page->vme->vaddr))
        {
            pagedir_set_accessed(page_thread->pagedir, lru_page->vme->vaddr, false);
            continue;
        }

        if (pagedir_is_dirty(page_thread->pagedir, lru_page->vme->vaddr) || lru_page->vme->type == VM_ANON)
        {
            if (lru_page->vme->type == VM_FILE)
            {
                // lock_acquire(&file_lock);
                file_write_at(lru_page->vme->file, lru_page->kaddr, lru_page->vme->read_bytes, lru_page->vme->offset);
                // lock_release(&file_lock);
            }
            else
            {
                lru_page->vme->type = VM_ANON;
                lru_page->vme->swap_slot = swap_out(lru_page->kaddr);
            }
        }

        lru_page->vme->is_loaded = false;

        pagedir_clear_page(page_thread->pagedir, lru_page->vme->vaddr);
        __free_page(lru_page);
        break;
    }

    lock_release(&lru_list_lock);
    return palloc_get_page(flags);
}