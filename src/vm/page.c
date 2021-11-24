#include <string.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"

static unsigned vm_hash_func(const struct hash_elem *, void * UNUSED);
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b);
static void vm_destroy_func(struct hash_elem *, void * UNUSED);


void vm_init(struct hash *vm)  /* hash table 초기화 */
{
    hash_init(vm, vm_hash_func, vm_less_func, NULL);     /* hash_init()으로 hash table 초기화 */
    /* 인자로 해시 테이블과 vm_hash_func과 vm_less_func 사용 */
}

void vm_destroy(struct hash *vm)   /* hash table 제거 */
{
    hash_destroy(vm, vm_destroy_func); /* hash_destroy()으로 hash table의 버킷리스트와 vm_entry들을 제거 */
}

struct vm_entry *find_vme(void *vaddr) /* 현재 프로세스의 주소공간에서 vaddr에 해당하는 vm_entry를 검색 */
{
    struct vm_entry vme;
    struct hash *vm = &thread_current()->vm;
    struct hash_elem *elem;

    vme.vaddr = pg_round_down(vaddr); /* pg_round_down()으로 vaddr의 페이지 번호를 구함 */
    /* hash_find() 함수를 사용해서 hash_elem 찾음 */
    if((elem = hash_find(vm, &vme.elem))) return hash_entry(elem, struct vm_entry, elem); /* hash_entry()로 해당 hash_elem의 vm_entry return */
    else return NULL;                                /* 만약 존재하지 않는다면 NULL 리턴 */
}

bool insert_vme(struct hash *vm, struct vm_entry *vme) /* hash table에 vm_entry 삽입 */
{
    if(!hash_insert(vm, &vme->elem)) return false;
    else return true;
}

bool delete_vme(struct hash *vm, struct vm_entry *vme) /* hash table에서 vm_entry 삭제 */
{
    if(!hash_delete(vm, &vme->elem)) return false;
    else {
        // free_page_vaddr(vme->vaddr);
        // swap_clear(vme->swap_slot);
        free(vme);
        return true;
    }
}

static unsigned 
vm_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
	return hash_int((int)vme->vaddr);
    /* hash_entry()로 element에 대한 vm_entry 구조체 검색 */
    /* hash_int()를 이용해서 vm_entry의 멤버 vaddr에 대한 hash key를 구하고 반환 */
}

static bool
vm_less_func(const struct hash_elem *a, const struct hash_elem *b)
{
    return hash_entry(a, struct vm_entry, elem)->vaddr < hash_entry(b, struct vm_entry, elem)->vaddr;
    /* hash_entry()로 각각의 element에 대한 vm_entry 구조체를 얻은 후 vaddr 비교(b가 크다면 true, a가 크다면 false */
}

static void
vm_destroy_func(struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    // free_page_vaddr(vme->vaddr);
    // swap_clear(vme->swap_slot);
    free(vme);
}

bool load_file(void *kaddr, struct vm_entry *vme)
{
    int read_byte = file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset);

	if (read_byte != (int) vme->read_bytes) return false;
	memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);

	return true;
}
static void merge()
{
    lock_acquire(&lru_lock);
    struct page *victim = isVictim();
    bool dirty = pagedir_is_dirty(victim->thread->pagedir, victim->vme->vaddr);
    switch(victim->vme->type)
    {
        case VM_BIN:
            if(dirty)
            {
                victim->vme->swap_slot = swap_out(victim->kaddr);
                victim->vme->type = VM_ANON;
            }
            break;
        case VM_FILE:
        if(dirty)
            {
                if(file_write_at(victim->vme->file,victim->vme->vaddr,victim->vme->read_bytes,victim->vme->offset)!=(int)victim->vme->read_bytes)
                    NOT_REACHED();
            }
            break;
        case VM_ANON:
            victim->vme->swap_slot = swap_out(victim->kaddr);
            break;
        default:
            NOT_REACHED();
    }
    victim->vme->is_loaded = false;
    pagedir_clear_page (victim->thread->pagedir, victim->vme->vaddr);
    del_page_from_lru_list(victim);
    palloc_free_page(victim->kaddr);
    free(victim);
    lock_release(&lru_lock);
}


struct page* alloc_page(enum palloc_flags flags)
{
    struct page *pg;
    pg = (struct page *)malloc(sizeof(struct page));
    if(!pg)
        return NULL;
    memset(pg,0,sizeof(struct page));
    pg->thread = thread_current();
    pg->kaddr = palloc_get_page(flags);
    while(pg->kaddr ==NULL)
    {
        merge();
        pg->kaddr = palloc_get_page(flags);
    }
    return pg;
}
//extern struct list lru_list; /*frame.c에 있는 global variable for gcc problem*/
void free_page_PM(void *kaddr)
{
    lock_acquire(&lru_lock);
    struct page *pg = find_page_lru(kaddr);
    if(pg!=NULL)
    {
        pagedir_clear_page (pg->thread->pagedir, pg->vme->vaddr);
        del_page_from_lru_list(pg);
        palloc_free_page(pg->kaddr);
        free(pg);
    }
    lock_release(&lru_lock);
}
void free_page_VM(void *vaddr)
{
    free_page_PM(pagedir_get_page(thread_current()->pagedir,vaddr));
}


