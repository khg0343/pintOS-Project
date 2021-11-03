#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

bool check_address (void *addr)
{
  // if(is_user_vaddr(addr)) {
  //   printf("yes it's valid\n");
  //   return true;
  // }
  // else {
  //   printf("yes it's invalid\n");
  //   return false;
  // }

  if (addr >= 0x8048000 && addr < 0xc0000000 && addr != 0) return true;
  else return false;
}

void get_argument(void *esp, int *arg, int count){
  int i;
  for(i = 0; i < count; i ++){
    if(!check_address(esp + 4*i)) sys_exit(-1);
    arg[i] = *(int *)(esp + 4*i);
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void
halt (void) 
{
  shutdown_power_off();
}

void
sys_exit (int status) {
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit ();
}

pid_t
exec (const char *file)
{
  pid_t pid = process_execute(file);
  if (pid == -1) return -1;

  struct thread* child = get_child_process(pid);
  if (!child) return -1;
  else {
    if (!child->isLoad) return -1;
  }

  return pid;
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size)
{
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
  
}

int
filesize (int fd) 
{
  
}

int
read (int fd, void *buffer, unsigned size)
{
  int i;
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  }
  return i;
}

int
write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

void
seek (int fd, unsigned position) 
{
  
}

unsigned
tell (int fd) 
{
  
}

void
close (int fd)
{
 
}

static void
syscall_handler (struct intr_frame *f ) 
{
  int argv[3];
  if (!check_address(f->esp)) sys_exit(-1);

  
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT: halt();
      break;
    case SYS_EXIT: 
      get_argument(f->esp + 4, &argv[0], 1);
      sys_exit((int)argv[0]);
      break;
    case SYS_EXEC:
      get_argument(f->esp + 4, &argv[0], 1);
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
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      get_argument(f->esp + 4, &argv[0], 3);
      f->eax = read((int)argv[0], (void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_WRITE:
      get_argument(f->esp + 4, &argv[0], 3);
      f->eax = write((int)argv[0], (const void*)argv[1], (unsigned)argv[2]);
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
    default :
      sys_exit(-1);
  }
}
