#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);
struct lock lock_file;

struct vm_entry *check_address(void* addr)
{
  if(addr < (void *)0x08048000 || addr >= (void *)0xc0000000) exit(-1);
  // if(!is_user_vaddr(addr)) exit(-1);
  
  return find_vme(addr);
  
  /*addr이 vm_entry에 존재하면 vm_entry를 반환하도록 코드 작성 */
  /*find_vme() 사용*/
} 

void check_valid_buffer(void *buffer, unsigned size, void *esp, bool to_write)
{
  int i;
  for (i = 0; i < size; i++)
    {
        struct vm_entry *vme = check_address(buffer + i);
        if (vme == NULL) exit(-1);
        if (to_write == true && vme->writable == false) exit(-1);
    }
  /* 인자로 받은 buffer부터 buffer + size까지의 크기가 한 페이지의 크기를 넘을 수도 있음 */
  /* check_address를 이용해서 주소의 유저영역 여부를 검사함과 동시에 vm_entry 구조체를 얻음 */
  /* 해당 주소에 대한 vm_entry 존재여부와 vm_entry의 writable 멤버가 true인지 검사 */
  /* 위 내용을 buffer 부터 buffer + size까지의 주소에 포함되는 vm_entry들에 대해 적용 */
}

void check_valid_string (const void* str)
{
  /* str에 대한 vm_entry의 존재 여부를 확인*/
  if(check_address(str) == NULL) exit(-1);

  /* check_address()사용*/
}

// bool check_address(void *addr)
// {
//   if(is_user_vaddr(addr)) return true;
//   else return false;
// }

void get_argument(void *esp, int *arg, int count){
  int i;
  for(i = 0; i < count; i++){
    if(!check_address(esp + 4*i)) exit(-1);
    arg[i] = *(int *)(esp + 4*i);
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock_file);
}

void
halt (void) 
{
  shutdown_power_off();
}

void
exit (int status) {
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit ();
}

pid_t
exec (const char *file)
{
  struct thread *child;
	pid_t pid = process_execute(file);
  if(pid==-1)  return -1;
	child = get_child_process(pid);
  sema_down(&(child->sema_load));      
	if(child->isLoad) return pid;
	else		return -1;
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size)
{
  if(file == NULL) exit(-1);
  return filesys_create(file, initial_size);
}

bool
remove (const char *file)
{
  return filesys_remove(file);
}

int
open (const char *file)
{
  int fd;
	struct file *f;

  if (file == NULL) exit(-1);

  lock_acquire(&lock_file); 
  f = filesys_open(file); /* 파일을 open */
  if (strcmp(thread_current()->name, file) == 0) file_deny_write(f);  /*ROX TEST*/
  
	if(f != NULL) { 
		fd = process_add_file(f);     /* 해당 파일 객체에 file descriptor 부여 */
    lock_release(&lock_file);
		return fd;                        /* file descriptor 리턴 */
	}
  lock_release(&lock_file);
	return -1; /* 해당 파일이 존재하지 않으면 -1 리턴 */
}

int
filesize (int fd) 
{
	struct file *f;
	if((f = process_get_file(fd))) { /* file descriptor를 이용하여 파일 객체 검색 */
		return file_length(f); /* 해당 파일의 길이를 리턴 */
	}
	return -1;  /* 해당 파일이 존재하지 않으면 -1 리턴 */
}

int
read (int fd, void *buffer, unsigned size)
{
  int read_size = 0;
	struct file *f;
  if(!check_address(buffer))
    exit(-1);
    
	lock_acquire(&lock_file); /* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */

	if(fd == 0) {   /* file descriptor가 0일 경우(STDIN) 키보드에 입력을 버퍼에 저장 후 버퍼의 저장한 크기를 리턴 (input_getc() 이용) */
    unsigned int i;
    for(i = 0; i < size; i++) {
       if (((char *)buffer)[i] == '\0') break;
    }
    read_size = i;
	} else {
		if((f = process_get_file(fd))) 
      read_size = file_read(f, buffer, size);  /* file descriptor가 0이 아닐 경우 파일의 데이터를 크기만큼 저장 후 읽은 바이트 수를 리턴 */
	}

	lock_release(&lock_file); /* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */

	return read_size;
}

int
write (int fd, const void *buffer, unsigned size)
{
  int write_size = 0;
	struct file *f;

	lock_acquire(&lock_file); /* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */

	if(fd == 1) { /* file descriptor가 1일 경우(STDOUT) 버퍼에 저장된 값을 화면에 출력후 버퍼의 크기 리턴 (putbuf() 이용) */
		putbuf(buffer, size);
		write_size = size;
	} else {    /* file descriptor가 1이 아닐 경우 버퍼에 저장된 데이터를 크기만큼 파일에 기록후 기록한 바이트 수를 리턴 */
		if((f = process_get_file(fd)))
			write_size = file_write(f,(const void *)buffer, size);
	}

	lock_release(&lock_file); /* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */

	return write_size;
}

void
seek (int fd, unsigned position) 
{
  struct file *f = process_get_file(fd); /* file descriptor를 이용하여 파일 객체 검색 */

	if(f != NULL) file_seek(f, position); /* 해당 열린 파일의 위치(offset)를 position만큼 이동 */
}

unsigned
tell (int fd) 
{
  struct file *f = process_get_file(fd); /* file descriptor를 이용하여 파일 객체 검색 */

	if(f != NULL) return file_tell(f); /* 해당 열린 파일의 위치를 return */
  return 0; 
}

void
close (int fd)
{
  process_close_file(fd);
}

mapid_t
mmap (int fd, void *addr)
{

}

void
munmap (mapid_t mapid)
{
  // struct mmap_file *f = find_mmap_file(mapid);
  // if (f != NULL) do_mummap (f);
}

static void
syscall_handler (struct intr_frame *f ) 
{
  int argv[3];
  if (!check_address(f->esp)) exit(-1);

  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT: halt();
      break;
    case SYS_EXIT: 
      get_argument(f->esp + 4, &argv[0], 1);
      exit((int)argv[0]);
      break;
    case SYS_EXEC:
      get_argument(f->esp + 4, &argv[0], 1);
      check_valid_string ((void *) argv[0]);
      f->eax = exec((const char*)argv[0]);
      break;
    case SYS_WAIT:
      get_argument(f->esp + 4, &argv[0], 1);
      f->eax = wait((pid_t)argv[0]);
      break;
    case SYS_CREATE:
      get_argument(f->esp + 4, &argv[0], 2);
      f->eax = create((const char*)argv[0], (unsigned)argv[1]);
      break;
    case SYS_REMOVE:
      get_argument(f->esp + 4, &argv[0], 1);
		  f->eax=remove((const char *)argv[0]);
      break;
    case SYS_OPEN:
      get_argument(f->esp + 4, &argv[0], 1);
      check_valid_string ((void *) argv[0]);
      f->eax = open((const char *)argv[0]);
      break;
    case SYS_FILESIZE:
      get_argument(f->esp + 4, &argv[0], 1);
		  f->eax = filesize(argv[0]);
      break;
    case SYS_READ:
      get_argument(f->esp + 4, &argv[0], 3);
      check_valid_buffer ((void *)argv[1], (unsigned)argv[2], f->esp, true);  //Changed
      f->eax = read((int)argv[0], (void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_WRITE:
      get_argument(f->esp + 4, &argv[0], 3);
      check_valid_buffer ((void *)argv[1], (unsigned)argv[2], f->esp, false);  //Changed
      f->eax = write((int)argv[0], (const void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_SEEK:
      get_argument(f->esp + 4, &argv[0], 2);
		  seek(argv[0],(unsigned)argv[1]);
      break;
    case SYS_TELL:
      get_argument(f->esp + 4, &argv[0], 1);
		  f->eax = tell(argv[0]);
      break;
    case SYS_CLOSE:
      get_argument(f->esp + 4, &argv[0], 1);
		  close(argv[0]);
      break;
    case SYS_MMAP:                   /* Map a file into memory. */
      get_argument(f->esp + 4, &argv[0], 2);
      f->eax = mmap(argv[0], (void*)argv[1]);
      break;
    case SYS_MUNMAP:                 /* Remove a memory mapping. */
      get_argument(f->esp + 4, &argv[0], 1);
      munmap(argv[0]);
      break;
    default :
      exit(-1);
  }
}