#include <hash.h>
#include <string.h>
#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"

static unsigned vm_hash_func(const struct hash_elem *, void *UNUSED);
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b);
static void vm_destroy_func(struct hash_elem *, void *UNUSED);

void vm_init(struct hash *vm) /* hash table 초기화 */
{
    hash_init(vm, vm_hash_func, vm_less_func, NULL); /* hash_init()으로 hash table 초기화 */
    /* 인자로 해시 테이블과 vm_hash_func과 vm_less_func 사용 */
}

void vm_destroy(struct hash *vm) /* hash table 제거 */
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
    if ((elem = hash_find(vm, &vme.elem)))
        return hash_entry(elem, struct vm_entry, elem); /* hash_entry()로 해당 hash_elem의 vm_entry return */
    else
        return NULL; /* 만약 존재하지 않는다면 NULL 리턴 */
}

bool insert_vme(struct hash *vm, struct vm_entry *vme) /* hash table에 vm_entry 삽입 */
{
    if (!hash_insert(vm, &vme->elem))
        return false;
    else
        return true;
}

bool delete_vme(struct hash *vm, struct vm_entry *vme) /* hash table에서 vm_entry 삭제 */
{
    if (!hash_delete(vm, &vme->elem))
        return false;
    else
    {
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

    if (read_byte != (int)vme->read_bytes)
        return false;
    memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);

    return true;
}

struct page *alloc_page(enum palloc_flags flags)
{
    struct page *page;
    page = (struct page *)malloc(sizeof(struct page));
    if (!page)
        return NULL;

    memset(page, 0, sizeof(struct page));
    page->thread = thread_current();
    page->kaddr = palloc_get_page(flags);

    try_to_free_pages(flags);

    add_page_to_lru_list(page);

    return page;
}
void free_page(void *kaddr)
{
    struct page *page = find_page_in_lru_list(kaddr);
    del_page_from_lru_list(page);
    palloc_free_page(page->kaddr);
    free(page);
}

void __free_page(struct page *page)
{
    /* LRU list 리스트 내 page 해제 */
    del_page_from_lru_list(page);
    palloc_free_page(page->kaddr);
    free(page);
}
