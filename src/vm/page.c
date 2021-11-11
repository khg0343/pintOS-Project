#include "page.h"

static unsigned vm_hash_func (const struct hash_elem *, void * UNUSED);
static bool vm_less_func (const struct hash_elem *, const struct hash_elem *, void * UNUSED);
static void vm_destroy_func (struct hash_elem *, void * UNUSED);


void vm_init (struct hash *vm)
{
/* hash_init()으로 해시테이블 초기화 */
/* 인자로 해시 테이블과 vm_hash_func과 vm_less_func 사용 */
}

void vm_destroy (struct hash *vm)
{
/* hash_destroy()으로 해시테이블의 버킷리스트와 vm_entry들을 제거 */
}


static unsigned vm_hash_func (const struct hash_elem *e,void *aux)
{
/* hash_entry()로 element에 대한 vm_entry 구조체 검색 */
/* hash_int()를 이용해서 vm_entry의 멤버 vaddr에 대한 해시값을 구하고 반환 */
}

static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b)
{
/* hash_entry()로 각각의 element에 대한 vm_entry 구조체를 얻은 후 vaddr 비교 (b가 크다면 true, a가 크다면 false */
}

static void
vm_destroy_func (struct hash_elem *e, void *aux UNUSED)
{
}

bool insert_vme (struct hash *vm, struct vm_entry *vme)
{
/* hash_insert()함수 사용 */
}


bool delete_vme (struct hash *vm, struct vm_entry *vme)
{
/* hash_delete()함수 사용 */
}

struct vm_entry *find_vme (void *vaddr)
{
/* pg_round_down()으로 vaddr의 페이지 번호를 얻음 */
/* hash_find() 함수를 사용해서 hash_elem 구조체 얻음 */
/* 만약 존재하지 않는다면 NULL 리턴 */
/* hash_entry()로 해당 hash_elem의 vm_entry 구조체 리턴 */
}


bool load_file (void *kaddr, struct vm_entry *vme)
{
}