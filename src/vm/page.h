#ifndef VM_PAGE_H
#define VM_PAGE_H

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

#include "vm/swap.h"
#include <hash.h>
#include "filesys/off_t.h"

void vm_init(struct hash *vm);  /* 해시테이블 초기화 */
void vm_destroy(struct hash *vm);   /* 해시테이블 제거 */
struct vm_entry *find_vme(void *vaddr); /* 현재 프로세스의 주소공간에서 vaddr에 해당하는 vm_entry를 검색 */
bool insert_vme(struct hash *vm, struct vm_entry *vme); /* 해시테이블에 vm_entry 삽입 */
bool delete_vme(struct hash *vm, struct vm_entry *vme); /* 해시 테이블에서 vm_entry삭제 */
bool load_file(void *kaddr, struct vm_entry *vme);

struct vm_entry{
    uint8_t type; /* VM_BIN, VM_FILE, VM_ANON의 type */
    void *vaddr; /* vm_entry가 관리하는 virtual page number */
    bool writable; /* True일 경우 해당 주소에 write 가능 ,False일 경우 해당 주소에 write 불가능 */
    bool is_loaded; /* physical page의 탑재 여부를 알려주는 flag */
    struct file* file; /* 가상주소와 맵핑된 파일 */

    /* Memory Mapped File 에서 다룰 예정 */
    struct list_elem mmap_elem; /* mmap 리스트 element */

    size_t offset; /* 읽어야 할 파일 오프셋 */
    size_t read_bytes; /* 가상페이지에 쓰여져 있는 데이터 크기 */
    size_t zero_bytes; /* 0으로 채울 남은 페이지의 바이트 */

    /* Swapping 과제에서 다룰 예정 */
    size_t swap_slot; /* 스왑 슬롯 */
    
    /* ‘vm_entry들을 위한 자료구조’ 부분에서 다룰 예정 */
    struct hash_elem elem; /* 해시 테이블 Element */
};

struct mmap_file {
    int mapid;
    struct file* file;
    struct list_elem elem;
    struct list vme_list;
};

#endif