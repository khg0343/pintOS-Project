#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

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
exit (int status) {
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit ();
}

pid_t
exec (const char *file)
{
  //printf("hello exec");
  return process_execute(file);
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
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT: halt();
      break;
    case SYS_EXIT: 
      exit((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_EXEC:
      printf("hello syscall handler switch exec");
      if(!is_user_vaddr(f->esp + 4)) {
        printf("exit! not valid in exec");
        exit(-1);
      }
      f->eax = exec((const char*)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_WAIT:
      if(!is_user_vaddr(f->esp + 4)) exit(-1);
      f->eax = wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CREATE:
      if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8)) exit(-1);
      f->eax = create((const char*)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8) || !is_user_vaddr(f->esp + 12)) exit(-1);
      f->eax = read((int)*(uint32_t *)(f->esp + 4), (const void*)*(uint32_t *)(f->esp + 8), (unsigned)*(uint32_t *)(f->esp + 12));
      break;
    case SYS_WRITE:
      if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8) || !is_user_vaddr(f->esp + 12)) exit(-1);
      f->eax = write((int)*(uint32_t *)(f->esp + 4), (const void*)*(uint32_t *)(f->esp + 8), (unsigned)*(uint32_t *)(f->esp + 12));
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
    default :
      exit(-1);
  }
}
