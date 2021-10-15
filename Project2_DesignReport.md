**CSED312 OS Lab 2 - User Program**
================

**Design Report**
----------------

<div style="text-align: right"> 20180085 송수민 20180373 김현지 </div>

------------------------------

# **Introduction**

이번 프로젝트의 목표는 주어진 Pintos 코드를 수정하여 User program이 실행될 수 있도록 하는 것이다. User program 실행을 위해 아래의 사항들을 구현해야 한다.

1.	Argument passing
2.	User memory access
3.	Process terminate messages
4.	System call
5.	Denying writes to executables


------------------------------

# **I. Anaylsis on Current Pintos system**

본 Lab 2에서는 PintOS의 User Program에 대해 주어진 과제를 수행한다.
주어진 과제를 수행하기 전 현재 pintOS에 구현되어 있는 system을 알아보자.

## **Process execution procedure**

## **System Call Procedure**

## **File System**

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