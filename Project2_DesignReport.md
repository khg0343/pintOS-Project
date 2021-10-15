**CSED312 OS Lab 2 - User Program**
================

**Design Report**
----------------

<div style="text-align: right"> 20180085 송수민 20180373 김현지 </div>

------------------------------

# **Introduction**

이번 프로젝트의 목표는 주어진 Pintos 코드를 수정하여 User program이 실행될 수 있도록 하는 것이다. User program 실행을 위해 아래의 사항들을 구현해야 한다.

1. Argument passing
2. User memory access
3. Process terminate messages
4. System call
5. Denying writes to executables


------------------------------

# **I. Anaylsis on Current Pintos system**

본 Lab 2에서는 PintOS의 User Program에 대해 주어진 과제를 수행한다.
주어진 과제를 수행하기 전 현재 pintOS에 구현되어 있는 system을 알아보자.

## **Process Execution Procedure**
먼저 PintOS의 Process Execution Procedure을 분석해보자.


```cpp
/* threads/init.c */
/* Pintos main program. */
int
main (void)
{
  char **argv;

  /* Clear BSS. */  
  bss_init ();

  /* Break command line into arguments and parse options. */
  argv = read_command_line ();
  argv = parse_options (argv);
  
  ...
  
  /* Run actions specified on kernel command line. */
  run_actions (argv);

  /* Finish up. */
  shutdown ();
  thread_exit ();
}
```

```cpp
/* threads/init.c */
/* Executes all of the actions specified in ARGV[] up to the null pointer sentinel. */
static void
run_actions (char **argv) 
{
  /* An action. */
  struct action 
    {
      char *name;                       /* Action name. */
      int argc;                         /* # of args, including action name. */
      void (*function) (char **argv);   /* Function to execute action. */
    };

  /* Table of supported actions. */
  static const struct action actions[] = 
    {
      {"run", 2, run_task},
#ifdef FILESYS
      {"ls", 1, fsutil_ls},
      {"cat", 2, fsutil_cat},
      {"rm", 2, fsutil_rm},
      {"extract", 1, fsutil_extract},
      {"append", 2, fsutil_append},
#endif
      {NULL, 0, NULL},
    };
  ...
  
}
```

```cpp
/* userprog/process.c */
/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  return tid;
}
```

```cpp
/* userprog/process.c */
/* A thread function that loads a user process and starts it running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
    thread_exit ();
  ...
}
```

```cpp
/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
  file = filesys_open (file_name);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  ... /* Read and verify executable header. */
  ... /* Read program headers. */

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}
```

## **System Call Procedure**
이번엔 PintOS의 System Call Procedure을 분석해보자.

System Call은 User Program의 Kernel 영역 접근을 위해 호출되는 kernel이 제공하는 interface이며, 이는 System Call Handler를 통해 이루어진다. System Call이 호출되면 kernel mode로 변환되며 kernel이 대신 해당 작업을 수행하고, 이후 다시 user mode로 변환되어 작업을 이어나간다.

먼저 이번 Project에서 다루어야할 System Call에는 어떤 종류가 있는지 System call number list를 통해 알아보자

```cpp
/* lib/syscall-nr.h */

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_HALT,                   /* Halt the operating system. */
    SYS_EXIT,                   /* Terminate this process. */
    SYS_EXEC,                   /* Start another process. */
    SYS_WAIT,                   /* Wait for a child process to die. */
    SYS_CREATE,                 /* Create a file. */
    SYS_REMOVE,                 /* Delete a file. */
    SYS_OPEN,                   /* Open a file. */
    SYS_FILESIZE,               /* Obtain a file's size. */
    SYS_READ,                   /* Read from a file. */
    SYS_WRITE,                  /* Write to a file. */
    SYS_SEEK,                   /* Change position in a file. */
    SYS_TELL,                   /* Report current position in a file. */
    SYS_CLOSE,                  /* Close a file. */
...
```

위의 13개의 System Call이 이번 Project2에서 다루어야하는 부분이며, 해당 기능들에 대한 함수는 아래의 user/syscall.c에 구현되어있다.
```cpp
/* lib/user/syscall.h*/
/* Projects 2 and later. */
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
```

그 예시로 exec를 이용하여 설명해보겠다.
```cpp
/* lib/user/syscall.c*/
pid_t
exec (const char *file)
{
  return (pid_t) syscall1 (SYS_EXEC, file);
}
```
> syscall의 함수는 매우 간단하게 구현되어있으며, 후술할 syscall macro에 system call number와 parameter인 file을 argument로 넘겨주는 것이 전부이다.

위에서 사용된 syscall1은 아래와 같은 형태로 정의되어있다
```cpp
/**/
/* Invokes syscall NUMBER, passing argument ARG0, and returns the
   return value as an `int'. */
#define syscall1(NUMBER, ARG0)                                           \
        ({                                                               \
          int retval;                                                    \
          asm volatile                                                   \
            ("pushl %[arg0]; pushl %[number]; int $0x30; addl $8, %%esp" \
               : "=a" (retval)                                           \
               : [number] "i" (NUMBER),                                  \
                 [arg0] "g" (ARG0)                                       \
               : "memory");                                              \
          retval;                                                        \
        })
```
> 이때 Number는 위에서 언급한 System call number를 의미하며, Arg0는 exec 함수의 parameter가 한 개이기 때문에 이를 담기 위한 값이다. system call number를 invoke하고, arg들은 user stack에 push한다. 여기서 $0x30은 kernel공간의 interrupt vector table에서 system call handler의 주소를 의미한다.

> syscall1은 arg가 1개인 경우에 대한 정의이며, arg개수에 따라 syscall0-3으로 나누어 사용하면 된다.


현재 pintOS의 system call handler인 syscall_handler()는 아래의 코드를 보다시피 아무것도 구현되어있지 않다. 이번 Project에서 user program이 요하는 기능을 user stack의 정보를 통해 수행할 수 있도록 해당 함수에 기능을 구현하여야 한다.

```cpp
/*userprog/syscall.c*/
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}
```

## **File System**
마지막으로 PintOS의 File System을 분석해보자.

먼저 PintOS의 file system에서 사용되는 struct를 보도록하자.
아래는 PintOS의 file의 구조체이다.
```cpp
/* filesys/file.c */
/* An open file. */
struct file 
{
  struct inode *inode;        /* File's inode. */
  off_t pos;                  /* Current position. */
  bool deny_write;            /* Has file_deny_write() been called? */
};
```
> inode : file system에서 file의 정보를 저장한다. </br>
> pos : file을 read 또는 write하는 cursor의 현재 postion을 의미한다. </br>
> deny_write : file의 write가능 여부를 표시하는 boolean 변수이다.

</br>

inode는 아래와 같은 정보를 저장하고 있는 구조체이다.
```cpp
/* filesys/inode.c */
/* In-memory inode. */
struct inode 
{
  struct list_elem elem;              /* Element in inode list. */
  block_sector_t sector;              /* Sector number of disk location. */
  int open_cnt;                       /* Number of openers. */
  bool removed;                       /* True if deleted, false otherwise. */
  int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  struct inode_disk data;             /* Inode content. */
};
```

inode의 data를 정의하는 data type인 inode_disk라는 구조체이다.

```cpp
/* filesys/inode.c */
/* On-disk inode. Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  block_sector_t start;               /* First data sector. */
  off_t length;                       /* File size in bytes. */
  unsigned magic;                     /* Magic number. */
  uint32_t unused[125];               /* Not used. */
};
```

> inode의 data는 inode_disk 구조체의 형태로 저장되어있으며, inode_disk는 저장한 data의 start부분의 sector의 index와, 그 file의 크기인 length를 저장하여 inode에 저장된 data를 정의하고 있다.

</br>

다음은 filesys에 포함된 다양한 함수를 보도록 하자.
```cpp
/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}
```

> filesys_create는 파일 이름이 name이고 크기가 initial_size인 file을 만드는 함수이다. </br>
> 간단히 내부를 보면 inode_sector를 선언하고 해당 sector에 대해 free_map_allocate함수를 이용하여 할당한다. 이후 inode_create함수를 이용하여 initial_size로 inode의 data length를 initialize하고 file system device의 sector에 새 inode를 쓴다. 이렇게 만들어진 inode에 대해 해당 sector가 파일명이 name인 file을 dir에 추가하는 과정을 통해 file이 create됨을 알 수 있다.

```cpp
/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}
```
> filesys_open는 파일 이름이 name인 file을 여는 함수이다. </br>
> dir_lookup을 통해 파일명이 name인 inode를 찾고, 해당 inode를 file_open하는 구조로 이루어져있다.

```cpp
/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}
```
> filesys_remove는 파일 이름이 name인 file을 삭제하는 함수이다. </br>
> dir_remove을 통해 파일명이 name인 inode를 찾아 삭제하는 구조로 이루어져있다.

User Program이 File System로부터 load되거나, System Call이 실행됨에 따라 file system에 대한 code가 필요하다. 하지만, 이번 과제의 초점은 file system을 구현하는 것이 아니기 때문에 이미 구현되어있는 pintOS의 file system의 구조를 파악하고 적절히 사용할 수 있도록 하여야 한다. pintOS 문서에 따르면 filesys에 해당하는 코드는 수정하지 않는 것을 권장하고 있으므로, 구현에 있어 주의하도록 한다.

그렇다면 각 thread가 이러한 file들에 어떻게 접근하고 사용하는 것은 어떻게 이루어질까?
이때 File Descriptor라는 개념이 사용된다. File Descriptor(fd)란 thread가 file을 다룰 때 사용하는 것으로, thread가 특정 file에 접근할 때 사용하는 추상적인 값이다. fd는 일반적으로 0이 아닌 정수값을 가지며 int형으로 선언한다. 
thread가 어떤 file을 open하면 kernel은 사용하지 않는 가장 작은 fd값을 할당한다. 이때, fd 0, 1, 2는 각각 stdin, stdout, stderr에 기본적으로 indexing되어있으므로 3부터 할당할 수 있다.
그 다음 thread가 open된 파일에 System Call로 접근하게 되면, fd값을 통해 file을 지칭할 수 있다.
각각의 thread에는 file descriptor table이 존재하며 각 fd에 대한 file table로의 pointer를 저장하고 있다. 이 pointer를 이용하여 file에 접근할 수 있게 되는 것이다.

현재 PintOS의 코드에는 이러한 부분이 구현되어있지 않다. 이번 Project에서 thread의 file 접근에 대한 기능을 수행할 수 있도록 fd에 대한 부분을 구현하여야 한다.

</br>

# **II. Process Termination Messages**
## **Solution**

>User Program이 종료되면, 종료된 Process의 Name과 어떠한 system call로 종료 되었는지에 대한 exit code를 출력한다. 출력 형식은 다음과 같다
  
  ```cpp
  printf("%s: exit(%d\n)",variable_1, variable_2)
  ```

>위 형식에서 variable_1은 Process의 Name이고, variable_2는 exit code 이다. 위는 Prototype으로 변수가 지정될 수 도 있고 directly하게 function을 call할 수도 있다. 각 요소를 어떻게 불러올지에 대해 알아보자.
### 1. Process Name
>Process Name은 process_execute(const char *file_name)에서 시작된다. 

  ```cpp
  /*userprog/process.c*/
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  ```

>process_execute에서 보면 *file_name을 parameter로 넘겨 받는다. 이를 thread를 생성할 때 argument로 넘기는데, thread structure에서 char name이라는 변수에 process Name을 저장한다. 즉, Process Name을 얻으려면 해당 thread에서 name을 얻어오는 method를 작성하면 된다.
### 2. Exit Code
> PintOS Document에서 구현해야 할 System call 중 exit을 보면 선언이 다음과 같이 되어 있다.

  ```cpp
  void exit(int status)
  ```

> Parameter인 status가 exit code이므로 exit안에서 exit code를 직접적으로 다룰 수 있다. 또한, thread.c의 thread_exit을 보면 thread_exit에서 process_exit을 call하는 것을 보아 종료 method call 순서는 thread_exit -> process_exit임을 알 수 있다. thread_exit은 system call : exit을 받으면 이 과정 중에서 실행 될 것이므로, exit method에서 위 형식의 message를 출력하는 것이 용이할 것이다. 위 Solution에 따라 구현하고자 하는 Brief Algorithm은 아래와 같다.

## **Brief Algorithm**
>System call : void exit(int status)에서 message를 출력한다. message에 담길 정보 중 process name은 thread structure에서 받아오는 method 하나를 구현하고 exit code는 exit에 넘겨준 status를 사용한다. 이때, 주의할 점으로는 kernel thread가 종료되거나 halt가 발생한 경우에는 process가 종료된 것이 아니므로 위 메세지를 출력하지 않아야 하는데 이 경우는 애초에 다른 exit()을 호출하지 않기 때문에 해결 된 issue이다.

## **To be Added / Modified**
- void exit(int status)
> Termination message를 출력하는 code를 추가한다.

- char* get_ProcessName(struct thread* cur)
> 종료되는 thread의 Name을 받아오는 method를 추가한다.


</br>

# **III. Argument Passing**

## **Analysis**

Add codes to support argument passing for the function process_execute(). Instead of taking a program file name as its only argument, process_execute() should split words using spaces and identify program name and its arguments separately. For instance, process_execute(“grep foo bar”) shoud run grep with two arguments foo and bar.

## **Solution**

Add codes to support argument passing for the function process_execute(). Instead of taking a program file name as its only argument, process_execute() should split words using spaces and identify program name and its arguments separately. For instance, process_execute(“grep foo bar”) shoud run grep with two arguments foo and bar.

</br>

# **IV. System Call**

## **Analysis**
Implement the system call handler in “userprog/syscall.c”. Current handler will terminate the process if system call is called. It should retrieve the system call number and system call arguments after the system call is handled.

## **Solution**

  Implement the following system calls:

**1. void halt(void)** </br>
    Terminates Pintos by calling shutdown_power_off()

**2. void exit(int status)**
    Terminates the current user program and return status to the kernel. Ther kernel passes the status to the parent process.
    
**3. pid_t exec(const char *cmd_line)**
Runs the program whose name is given in cmd_line and returns the new process’s pid. If its fails to execute the new program, it should return -1 as pid. Synchronization should be ensured for this system call.

**4. int wait (pid_t pid)**
Waits for the child process given as pid to terminate and retrieves the exit status. It is possible for the parent process to wait for a child process that has already been terminated. The kernel should retrieve the child’s exit status and pass it to the parent anyway. If the child process has been terminated by the kernel, return status must be -1.
Wait must fail and return -1 immediately if any of the following conditions is true:
- Pid does not refer to a direct child of the calling process. That is, if A spawns child B and B spawns child C, A cannot wait for C.
- The process already called wait for the pid in the past. That is, the process can wait for a pid only once.
Processes may spawn any number of children. Your design should consider all the possible situation that can happen between parent and the child process.
Implementing this system call requires considerably more work than any of the rest.

**5. bool create(const char *file, unsigned initial_size)**
Creates a new file with the name file and initialize its size with initial_size. Return true if successful, false otherwise.

**6. bool remove(const char *file)**
Deletes the file called file. Returns true if successful, false otherwise. A file may be removed whether it is open or closed. However, removing an open file does not close it.

**7. int open(const char *file)**
open the file with name file. Returns a nonnegative integer number for the file descriptor, or -1 if unsuccessful.
File descriptors number 0 and 1 are reserved for the console, STDIN_FILENO, STDOU”T_FILENO, respectively. These numbers should not be used.
Each process has an independent set of file descriptors and file descriptors are not inherited to child processes.

**8. int filesize(int fd)**
Returns the size of opened file with fd.

**9. int read(int fd, void *buffer, unsigned size)**
Reads size bytes from the opend file and save the contents into buffer. Returns the number of bytes that are acutally read. -1 should be returned if the system fails to read. If 0 is given as fd, it should read from the keyboard using input_getc()

**10. int write(int fd, const void *buffer, unsigned size)**
Writes size bytes from buffer to the open file fd. Returns the number of bytes actually written.
Since the basic file system for this project does not support file growth, you should not write past the end-of-file.
If 1 is given as the fd, it should write to the console. You should use putbuf() to write things in the buffer to the console.

**11. void seek (int fd, unsigned position)**
Changes the next bytes to be read or written in open file fd to position.

**12. unsigned tell (int fd)**
Returns the position of the next byte to be read or written in open file fd.

**13. void close (int fd)**
Closes the file with fd. You should also close all of its open file descriptors as well.

# **V. Denying Writes to Executables**

## **Analysis**

Add code to deny writes to files in use as executable. You can use file_deny_write() to prevent writes to an open file. Calling file_allow_write() or closing a file will re-enable writes to the file.

## **Solution**
Add code to deny writes to files in use as executable. You can use file_deny_write() to prevent writes to an open file. Calling file_allow_write() or closing a file will re-enable writes to the file.