#include <list.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

struct list lru_list;
struct lock lru_lock;
struct list_elem *lru_clock;
static struct list_elem* get_next_lru_clock();
void lru_list_init(void)
{
    list_init(&lru_list);
    lock_init(&lru_lock);
    lru_clock = NULL;
}

void add_page_to_lru_list(struct page* page)
{
    lock_acquire(&lru_lock);
    list_push_back(&lru_list,&page->lru);
    lock_release(&lru_lock);
}
void del_page_from_lru_list(struct page* page)
{
    if (lru_clock == &page->lru)
    {
      lru_clock = list_remove (lru_clock);
    }
  else
    {
      list_remove (&page->lru);
    }

}

static struct list_elem* get_next_lru_clock()
{
    if (lru_clock == NULL || lru_clock == list_end (&lru_list))
    {
      if (list_empty (&lru_list))
        return NULL;
      else
        return (lru_clock = list_begin (&lru_list));
    }
  lru_clock = list_next (lru_clock);
  if (lru_clock == list_end (&lru_list))
      return get_next_lru_clock ();
  return lru_clock;
}
struct page* find_page_lru(void *kaddr)
{
    struct list_elem *ele;
    for(ele = list_begin(&lru_list); ele!=list_end(&lru_list); ele=list_next(ele))
    {
        struct page *pg = list_entry(ele,struct page,lru);
        if(pg->kaddr == kaddr)
            return pg;
    }
    return NULL;
}

struct page* isVictim()
{
    struct page* pg;
    struct list_elem *ele;
    ele = get_next_lru_clock();
    pg = list_entry(ele,struct page, lru);
    while(pg->vme->victed || pagedir_is_accessed(pg->thread->pagedir, pg->vme->vaddr))
    {
        pagedir_set_accessed(pg->thread->pagedir, pg->vme->vaddr,false);
        ele = get_next_lru_clock();
        pg = list_entry(ele,struct page, lru);
    }
    return pg;
}