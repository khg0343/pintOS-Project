#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/syscall.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

void construct_esp(char *file_name, void **esp);
struct thread *get_child_process (pid_t pid);
void remove_child_process(struct thread *cp);

#endif /* userprog/process.h */
