**CSED312 OS Lab 2 - User program**
================

**Final Report**
----------------

<div style="text-align: right"> 20180085 송수민 20180373 김현지 </div>

------------------------------

# **I. Implementation of Argument Passing**

## **Analysis**

현재 pintOS의 Argument Passing은 구현되어 있지 않다. pintos –q run ‘echo x’ 라는 명령어를 입력하면 ‘echo x’라는 argument들을 메모리에 쌓아 process가 이를 활용할 수 있도록 해야하는데, 이러한 기능이 구현되어 있지 않은 것이다. 궁극적으로, 이번 Implementation of Argument Passing은 문자열로 들어온 total argument들을 공백을 기준으로 구분해서 메모리에 쌓는 것을 목적으로 한다. 

## Brief Algorithm

- 1.    Process_execute()가 실행되면 file_name의 첫 부분(token이라 명명)을 thread_create에 첫번째 인자로 넘긴다. 
- 2.	Start_process에서 load를 호출한다. 이때, load의 첫번째 parameter를 file_name의 첫번째 token을 넘긴다.
- 3.	Load()가 success를 return하면 putArguments(file_name,&if_.esp)를 호출하여 메모리에 argument, address of argument, return address를 넣는다.


## Implementation

- process_execute() 변경 사항이다.
  
```cpp
tid_t process_execute (const char *file_name) 
{ 
...
  char *fn_copy_2 = palloc_get_page(0);
  strlcpy(fn_copy_2,file_name,PGSIZE);
  char *name;
  char *remain;
  name = strtok_r(fn_copy_2," ",&remain);
  /* Create a new thread to execute FILE_NAME. */

  tid = thread_create (name, PRI_DEFAULT, start_process, fn_copy);
  palloc_free_page(fn_copy_2);
...
}
```
> thread_create()를 호출 할 때 기존에는 file_name 전체를 parameter로 넘겼다. 이는 후에 exit system call 시에 name을 불러오는데 잘못된 name을 불러오기 때문에 변경하여야 한다. 변경방법은 직접적으로 file_name을 접근하지 못하게 하기 위해 fn_copy_2를 새로 동적할당하여 file_name을 복사하고 이를 strtok_r을 이용하여 문자열을 잘라 첫 부분만 thread_create에 넘긴다. 이후, OS는 효율적인 메모리 관리가 중요하므로 필요없는 fn_copy_2를 해제한다.

- process_start() 변경 사항이다.

```cpp
static void start_process (void *file_name_)
{
...
  char* fn_copy_1 = palloc_get_page(0);
  char* cmd_name;
  char *remain;
  strlcpy(fn_copy_1,file_name,PGSIZE);
  cmd_name = strtok_r(fn_copy_1," ",&remain);
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (cmd_name, &if_.eip, &if_.esp);
  //printf("value of success : %d\n",success);
  if(success){
    construct_esp(file_name, &if_.esp);
  }
  //printf("Checking Memory\n");
  //hex_dump(if_.esp, if_.esp, PHYS_BASE - if_.esp,true);
  palloc_free_page(fn_copy_1);
}
```
> process_execute()의 변경사항과 동일하다. load에서 프로그램 명으로 disk에서 file을 open하기 때문에, 'echo x'에서 'echo'만을 넘겨주어야 한다. 동일한 방법으로 load 첫번째 인자에 cmd_name을 넘겨준다. 이후, load가 true를 return하면 성공적으로 load되었다는 것이므로 construct_esp()를 호출하여 Stack에 argument를 쌓아준다. 이때 넘기는 parameter는 전체 file_name과 esp이다. construct_esp()가 종료되면 fn_copy_1을 해제한다.

- construct_esp()의 구현이다.
  
```cpp
void construct_esp(char *file_name, void **esp) 
{
  char ** argv;
  int argc;
  int total_len;
  char stored_file_name[256];
  char *token;
  char *last;
  int i;
  int len;
  
  strlcpy(stored_file_name, file_name, strlen(file_name) + 1);
  token = strtok_r(stored_file_name, " ", &last);
  argc = 0;
  /* calculate argc */
  while (token != NULL) 
  {
    argc += 1;
    token = strtok_r(NULL, " ", &last);
  }
  argv = (char **)malloc(sizeof(char *) * argc);
  /* store argv */
  strlcpy(stored_file_name, file_name, strlen(file_name) + 1);
  for (i = 0, token = strtok_r(stored_file_name, " ", &last); i < argc; i++, token = strtok_r(NULL, " ", &last)) {
    len = strlen(token);
    argv[i] = token;
  }
  /* push argv[argc-1] ~ argv[0] */
  total_len = 0;
  for (i = argc - 1; 0 <= i; i --) 
  {
    len = strlen(argv[i]);
    *esp -= len + 1;
    total_len += len + 1;
    strlcpy(*esp, argv[i], len + 1);
    argv[i] = *esp;
  }
  /* push word align */
  *esp -= total_len % 4 != 0 ? 4 - (total_len % 4) : 0;
  /* push NULL */
  *esp -= 4;
  **(uint32_t **)esp = 0;
  /* push address of argv[argc-1] ~ argv[0] */
  for (i = argc - 1; 0 <= i; i--) 
  {
    *esp -= 4;
    **(uint32_t **)esp = argv[i];
  }
  /* push address of argv */
  *esp -= 4;
  **(uint32_t **)esp = *esp + 4;
  /* push argc */
  *esp -= 4;
  **(uint32_t **)esp = argc;
  /* push return address */
  *esp -= 4;
  **(uint32_t **)esp = 0;
  free(argv);
}
```
> 메모리에 argument를 쌓는 전체적인 과정은 esp를 직접 조작하여 쌓는다. Stack은 위에서 아래로 자라나므로, -연산을 이용하여 조작한다. Stored_file_name은 file_name을 이용하여 직접 token을 조작하면 원본이 바뀔 수 있기 때문에 만든 일종의 temp value이다. Size를 256으로 고정한 이유는 pintos document에서 256보다 큰 argument는 들어올 수 없다고 제한을 걸어놓았고, 이를 이용해도 된다는 글에서 착안했다. 
> 먼저, strlcpy를 통해 복사를 하여 argument의 숫자부터 count한다. 이후 argc를 이용하여 argv를 argc만큼 동적할당한다. 다시 strlcpy를 통해 token을 잘라 이번엔 argv[i]에 해당 argument를 넣는다. 이 argv를 이용하여 stack에 쌓는다. 현재 esp는 setup_stack의 초기화로 인해 PHYS_BASE이며 아래로 자라나기 때문에 가장 마지막 argument부터 차례로 넣어준다. for문에서 argc-1가 initial value인 이유이다. 
> 4의 배수에 맞게 word alignment를 실행한 이후 다시 argv[i]의 address를 차례로 넣어준다. Argv의 address를 넣고 argc, fake return address까지 넣어준 후 argv를 해제해준다.
```cpp
bfffffe0  00 00 00 00 02 00 00 00-ec ff ff bf f9 ff ff bf |................|
bffffff0  fe ff ff bf 00 00 00 00-00 65 63 68 6f 00 78 00 |.........echo.x.|
```
- 위는 hex_dump(if_.esp, if_.esp, PHYS_BASE – if_.esp, true)를 실행한 결과이다. 알맞게 stack에 쌓인 것을 확인 할 수 있다.
  ★★★★★★★★★★★★★construct_esp 고치고 다시 수정 필요 ★★★★★★★★★★★★★★★★


</br></br></br></br></br></br></br></br></br></br></br></br>
</br></br></br></br></br></br></br></br></br></br></br></br>

# **II. Implementation of Priority Scheduling**

## **Analysis**

현재 구현되어 있는 thread 구조체에 member 변수로 priority가 있다. 이를 어떻게 사용하는지 알아보기 위해 Scheduling이 실행되는 yield, unblock 함수를 살펴보자.

```cpp
void thread_unblock (struct thread *t) 
{
    enum intr_level old_level;
    ASSERT (is_thread (t));
    old_level = intr_disable ();
    ASSERT (t->status == THREAD_BLOCKED);
    list_push_back (&ready_list, &t->elem);
    t->status = THREAD_READY;
    intr_set_level (old_level);
}  

void thread_yield (void) 
{
    struct thread *cur = thread_current ();
    enum intr_level old_level;
    ASSERT (!intr_context ());
    old_level = intr_disable ();
    if (cur != idle_thread) 
        list_push_back (&ready_list, &cur->elem);
    cur->status = THREAD_READY;
    schedule();
    intr_set_level (old_level);
}
```

list_push_back을 통해, priority는 고려하지 않고 들어오는 순서대로 ready_list에 뒤에 넣는 것을 볼 수 있다. 이제 Priority를 고려하여 ready_list에 넣고자 한다.

## **Brief Algorithm**

Scheduling시에 Priority를 고려하여 ready_list에 넣는다. 또한, thread를 create하거나 priority를 재설정 하였을 때 현재 실행되고 있는 thread의 priority와 비교하여 더 높다면 즉시 yield한다.

</br></br></br></br></br></br></br></br>

## **Implementation**

-Scheduling시 ready_list에 넣는 과정이 있는 method들에, list_push_back 대신 list_insert_ordered 사용하여 element를 list에 넣도록 구현하였다.
```cpp
void thread_unblock (struct thread *t) 
{
    enum intr_level old_level;
    ASSERT (is_thread (t));
    old_level = intr_disable ();
    ASSERT (t->status == THREAD_BLOCKED);
    list_insert_ordered(&ready_list, &t->elem, thread_comparepriority, NULL);
    //list_push_back (&ready_list, &t->elem);
    t->status = THREAD_READY;
    intr_set_level (old_level);
}
void thread_yield (void) 
{
    struct thread *cur = thread_current ();
    enum intr_level old_level;
    ASSERT (!intr_context ());
    old_level = intr_disable ();
    if (cur != idle_thread)
        list_insert_ordered(&ready_list, &cur->elem, thread_comparepriority, NULL); // 0924
        //list_push_back (&ready_list, &cur->elem);
    cur->status = THREAD_READY;
    schedule ();
    intr_set_level (old_level);
}
```

> 기존에는 list_push_back으로 먼저 들어온 thread가 별도의 priority에 관한 순서 없이 실행이 되게 설계가 되어 있었는데, 이를 priority를 기준으로 정렬되게 list_insert_ordered로 대체하여 준다.

- 이때, Alarm-clock에서 사용하였던 비교함수처럼 thread_comparepriority를 선언 및 정의하여 사용한다. 이 함수에 관한 내용은 뒤에 후술한다. 그 외 변경사항은 없다.

```cpp
bool thread_comparepriority(struct list_elem *thread_1, struct list_elem *thread_2, void *aux)
{
    return list_entry(thread_1, struct thread, elem)->priority > list_entry(thread_2, struct thread, elem) -> priority;
}
```

>전체적인 Logic은 비슷하나, priority가 큰 entry가 앞에 위치하여야 하기 때문에 부등호가 반대로 바뀌었다. 또한, 위에 서술한 동일한 이유로 등호는 붙이지 않는다.

</br></br>

- Priority를 고려하여 줄 상황이 2가지 더 존재한다. Priority를 재설정하거나, thread를 create하여 새로운 thread와 기존의 thread들 간의 prioirty를 비교해줄 필요가 있는 상황이다. thread_create와 thread_set_priority에서 나타나며, thread_compare()가 필요한 상황에 함수를 호출하였다.

```cpp
void thread_set_priority (int new_priority) {
    struct thread *thrd_cur = thread_current();
    if(!thread_mlfqs){
        thrd_cur->origin_priority = new_priority;
        reset_priority(thrd_cur);
        thread_compare(); // priority설정 후, priority에 따라 thread yield
    }
}
tid_t thread_create (const char *name, int priority, thread_func *function, void *aux) {
    …
    thread_unblock (t); // Thread 생성완료 직전, Unblock하여 Ready Queue에 넣는다.
    thread_compare();
    return tid;
}
```

>먼저, thread_set_priority를 보면 thread_compare()를 볼 수 있다. 위 2줄은 priority donation을 위한 code 구현으로 뒤에서 설명한다. 대략적으로 설명하면, 현재 thread의 priority를 재설정한다. 재설정 후에 기존 thread ready_list에 있는 thread가 재설정된 priority보다 크다면 바로 해당 thread에 yield해주어야 한다. 이는 thread_compare()에서 실행한다. 이 함수는 바로 뒤에 설명한다. Thread_create()도 같은 맥락으로 새로운 thread가 생성되었으므로 생성된 thread의 priority와 기존 thread들의 prioirity를 비교하여 생성된 thread의 priority가 더 크다면 바로 이 thread에 CPU를 내주어야 한다.

- 현재 thread와 ready_list의 top thread와 비교하여 ready_list에 있는 thread의 priority가 더 크다면 thread_yield를 호출 함수를 구현하였다.

```cpp
void thread_compare() { //create될때랑 priority 재설정 할때.
    if(!list_empty(&ready_list) && (thread_current()->priority < list_entry(list_front(&ready_list),struct thread, elem)->priority))
        thread_yield();
}
```

>Thread_compare에서 현재 thread와 ready_list의 top thread와 비교하여 ready_list에 있는 thread의 priority가 더 크다면 thread_yield를 호출한다. 여기서 주의할 점은 thread_set_priority는 thread_current와 직접적으로 비교하는 것이어서 직관적으로 작동 흐름이 보이나, create의 경우 current를 지정하는 과정이 없어서 주의할 필요가 있다. Create의 경우 생성된 thread의 priority가 ready_list에 있는 thread들의 priority 중에서 가장 크다면 ready_list의 top에 저장될 것이다. 이는 thread_unblock에서 이루어진다. 이후, thread_compare()가 실행되고, create될 때 실행 되고 있는 thread와 create되어 넣어진 thread를 비교하여, create된 thread의 priority가 더 크다면 바로 CPU를 내어주는 Logic이다.

위 방법으로 thread의 priority를 고려하여 thread scheduling을 완성 할 수 있다.

# **III. Implementation of Priority Donation**

## **Analysis**

세 개의 thread가 있을 때, 각각의 thread의 priority를 비교하여 가장 높은 thread가 H, 다음이 M, 가장 작은 thread를 L이라 하자. 이때, H가 lock을 요청했을때 이에 대한 lock이 L의 소유일 경우 H가 L에게 점유를 넘겨주면 L보다 우선순위가 높은 M이 점유권을 먼저 선점하게 되어 M L H 순서로 thread가 마무리 되는데, 이렇게 되면  M이 H보다 우선순위가 낮음에도 불구하고 먼저 실행되는 문제가 생긴다. 이를 priority inversion라고 한다.

이를 해결하는 방법으로 priority donation이 있다. 이는 H가 자신이 가진 priority를 L에게 일시적으로 넘겨 동등한 priority조건에서 점유하도록하여, 위와 같은 문제가 생기지 않도록 하는 방법이다.
아래는 pintOS에서 제공하는 두 가지 donation과 관련된 문제이다.

>**Multiple donation**
>
>- 하나의 thread에 donation이 여러번 일어난 상황
>- Priority가 낮은 thread가 여러개의 우선순위가 더 높은 thread들의 lock을 모두 점유하고 있을 경우, Priority를 여러번 Donation받기 때문에 여러개의 Priority로 변경될 수 있는 데, 이때는 그 중 가장 높은 Priority로 갖도록 한다.

>**Nested donation**
>
>- lock P를 점유하고 있는 thread B가 다른 lock Q를 점유하고 있는 thread A에 lock Q를 신청하여, thread C가 thread B에 lock P를 신청할 경우 그 lock Q를 기다리는 것을 같이 기다려야하는 상황  

위와 같은 문제로 인해 Donation한 thread들의 list와 Lock에 대한 정보를 저장해야하며, Donation이후 초기 Priority로 돌아오기 위해 이에 대한 값을 모두 thread 내부적으로 저장하여야 한다.

## **Brief Algorithm**

초기 priority를 뜻하는 origin_priority 및 donation list, wait중인 lock에 대한 변수 등을 thread에 추가한다.
priority를 donate하는 함수와, 기존의 priority로 reset시키는 함수를 구현해야하며, lock_acquire될 때 조건에 따라 donation이 진행되고, lock_release될 때 reset이 진행되도록 한다. 이때 priority값 뿐만아니라 donation list, wait lock에 대한 값도 함께 수정해주어야한다.

</br></br></br></br></br></br></br></br></br></br></br></br></br>

## **Implementation**

- origin_priority 및 donation list, wait중인 lock에 대한 변수 등을 thread에 추가하고, 초기화하였다.

```cpp
//threads/thread.h
struct thread
{
    ...
   int origin_priority;             //donation이전의 기존 priority
   struct list donation_list;       //thread에 priority를 donate한 thread들의 list
   struct list_elem donation_elem;  //위 list를 관리하기 위한 element
   struct lock *wait_lock;          //이 lock이 release될 때까지 thread는 기다린다.
    ...
}
```

```cpp
//threads/thread.c
static void
init_thread (struct thread *t, const char *name, int priority)
{
  ...
  t->origin_priority = priority;
  list_init(&t->donation_list);
  t->wait_lock = NULL;
  ...
}
```

- priority를 donate하는 함수(donate_priority)와, 기존의 priority로 reset시키는 함수(reset_priority)를 구현하였다.

```cpp
//threads/synch.c
void donate_priority(struct thread *thrd)
{
  enum intr_level old_level;
  old_level = intr_disable();

  int level;

  //In Pintos Document, we can apply depth of nested priority donation, level 8)
  for (level = 0; level < LEVEL_MAX; level++) 
  {
    if (!thrd->wait_lock) break;
    struct thread *holder = thrd->wait_lock->holder;
    holder->priority = thrd->priority;
    thrd = holder;
  }
  intr_set_level(old_level);
}
```

> pintOS 공식문서에 따르면 nested priority donation의 최대 깊이는 8로, chain처럼 연결된 donation list를 순회하며 priority donation을 실행한다. thrd_cur의 wait_lock이 없을때까지 donation chain의 말단의 thread부터 차례로 donation이 이루어진다.

```cpp
//threads/synch.c
void reset_priority(struct thread *thrd)
{
  enum intr_level old_level;
  old_level = intr_disable();

  ASSERT(!thread_mlfqs);

  thrd->priority = thrd->origin_priority;

  if (!list_empty(&thrd->donation_list))
  {
    list_sort(&thrd->donation_list, thread_comparepriority, NULL);
    struct thread *front = 
    list_entry(list_front(&thrd->donation_list), struct thread, donation_elem);
    
    if (front->priority > thrd->priority) thrd->priority = front->priority;
  }

  intr_set_level(old_level);
}
```
> donation이후 wait_lock이 release되면, priority donate받은 thread는 priority를 다시 원래의 값으로 변경되어야하기에, thread에 저장되어있는 origin_priority로 바꾸어준다. 이때 multiple donation이 일어난 경우, donation_list에 thread가 남아있기 때문에 이를 확인하고, donation list의 priority를 확인하여 높은 순으로 정렬한 후 가장 priority가 높은 thread가 해당 thread보다 priority가 더 높을 경우, 다시 donation을 받아 priority를 설정하도록 한다.


- 위에서 구현한 donate_priority함수를 이용하여 lock_acquire될 때 조건에 따라 donation이 진행되도록 구현하였다.

```cpp
//threads/synch.c
void lock_acquire(struct lock *lock)
{
    ...
    if (lock->holder) {
        thrd_cur->wait_lock = lock;
        list_insert_ordered(&lock->holder->donation_list, 
                        &thrd_cur->donation_elem, thread_comparepriority, NULL);
        donate_priority(thrd_cur);
    };
    ...
    thrd_cur->wait_lock = NULL;
}
```

> 해당 lock에 대해 holder가 이미 존재할 경우 즉, lock이 다른 thread에 holding되어있으면 당장 해당 lock을 사용할 수 없다. 따라서 현재 thread의 wait_lock에 lock을 설정해주고, 현재 thread를 해당 thread의 donation list에 추가한다. 이때 priority를 비교하여 sorting한 후 넣어준다. 이후 현재 thread에 대해 위에서 구현한 donate_priority 함수를 이용하여 priority donation을 수행한다.

</br>

- 위에서 구현한 reset_priority함수를 이용하여 lock_release될 때 reset이 진행되도록 구현하였다.
  
```cpp
//threads/synch.c
void lock_release(struct lock *lock)
{
    ...
    lock_remove(lock);
    reset_priority(lock->holder);
    ...
}
```

> lock을 release함에 따라, 현재 thread의 donation list에서 해당 lock을 wait하고 있던 thread들을 donation list에서 제거하는 함수를 호출하고, lock을 holding하고 있던 thread의 priority를 기존의 값으로 설정하며, multiple donation으로 인해 priority를 재설정해야하는 경우가 있을 수 있으므로, 위에서 구현한 reset_priority 함수를 이용하여 priority reset을 수행한다. 이후, lock의 holder를 NULL로 하여 lock을 최종적으로 release한다.

</br>

- 이 때 현재 thread의 donation list에서 해당 lock을 wait하고 있던 thread들을 donation list에서 제거하는 함수(lock_remove)를 구현하였다.

```cpp
//threads/synch.c
void lock_remove(struct lock *lock)
{
  ASSERT(!thread_mlfqs);

  struct thread *thrd_cur = thread_current();
  struct list *list = &thrd_cur->donation_list;
  struct list_elem *e;

  for (e = list_begin(list); e != list_end(list); e = list_next(e))
  {
    struct thread *thrd = list_entry(e, struct thread, donation_elem);
    if (lock == thrd->wait_lock)
      list_remove(e);
  }
}
```

</br>

- thread unblock후 unblock된 thread가 running thread보다 priority가 높을 수 있으므로 compare함수를 호출해주었다.

```cpp
void sema_up(struct semaphore *sema)
{
  ...
  thread_compare(); 
  ...
}
```

- Synchronization 중 Conditional Variable에 해당하는 method이다.
  
```cpp
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
}
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  // list_push_back (&cond->waiters, &waiter.elem);
  list_insert_ordered(&cond->waiters, &waiter.elem, sema_comparepriority, NULL);
   //sema compare priority에 따라 push back이 아닌 order하게 insert
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    list_sort(&cond->waiters, sema_comparepriority, NULL);
    sema_up(&list_entry(list_pop_front(&cond->waiters), struct semaphore_elem, elem)->semaphore);
  }
}

bool sema_comparepriority(const struct list_elem *thread_1, const struct list_elem *thread_2, void *aux)
{
  struct semaphore_elem *sema_1 = list_entry(thread_1, struct semaphore_elem, elem);
  struct semaphore_elem *sema_2 = list_entry(thread_2, struct semaphore_elem, elem);
  struct list *waiter_1 = &(sema_1->semaphore.waiters);
  struct list *waiter_2 = &(sema_2->semaphore.waiters);
  struct list_elem *elem_1 = list_begin(waiter_1);
  struct list_elem *elem_2 = list_begin(waiter_2);

  return list_entry(elem_1, struct thread, elem)->priority > list_entry(elem_2, struct thread, elem)->priority;
}
```
>cond_wait이 호출되면 기존에는 list_push_back으로 들어오는 순서대로 뒤에 저장이 되었다. 이를 위와 마찬가지로 priority를 고려하여 list에 넣어준다. Conditional Variable은 Lock, Semaphore와 조금은 달리 semaphore_elem라는 구조체를 이용한다. Conditional Variable을 이용하여 여러 thread에 걸려있는 semaphore를 해제 및 할당 할 수 있기 때문에 이를 전체적으로 관리하는 semaphore_elem 구조체를 사용한다. 먼저 cond_wait에 호출되면 초기화를 거친 후 semaphore들의 가장 앞에 있는 thread의 priority를 비교하여 list에 넣어준다. 그 이후 sema_down에 가서 반복문에 계속 loop에 걸려있는다. 이후, cond_signal을 보내면은 loop가 걸려있는 동안 priority가 변경되었을 수도 있으므로 list_sort를 진행하고, 가장 앞에있는 entry를 pop시켜준다.
  
</br></br></br></br></br></br></br></br></br></br></br>
</br></br></br></br></br></br></br></br></br></br></br>

# **IV. Implementation of Advanced Scheduler**

## **Analysis**

위에서 계속 사용하던 Priority Scheduling이 아닌 정해진 수식에 따라 thread의 priority를 계산해주는 Scheduling 기법을 Multi-Level Feedback Queue Schedulling(MLFQS)라 한다. 실행시에 -mlfqs 옵션에 따라 실행되며, 동시에 전역으로 선언된 thread_mlfqs boolean 값이 true로 setting된다. 이 변수값에 따라 실행되는 함수의 코드를 적절하게 작성해야한다.

아래의 세 변수는 MLFQS mode에서 priority를 결정하기 위한 변수들의 계산식이다.
|||
|------|---|
|**priority**|- priority : PRI_MAX - (recent_cpu/4) - (nice*2) </br> - 4 tick 마다 모든 thread의 priority를 다시 계산|
|**recent_cpu**|- recent_cpu : (2 * load_avg) / (2 *load_avg + 1)* recent_cpu + nice </br> - 최근에 thread가 CPU time 을 얼마나 많이 사용했는지를 나타내는 값 </br> - 1 tick 마다 running thread의 recent_cpu이 1씩 증가 </br> - 1초 마다 모든 thread의 recent_cpu를 다시 계산 </br> - 실수값|
|**load_avg**|- load_avg : (59/60) * load_avg + (1/60) *ready_threads</br>(*이 때 ready_threads는 ready or running thread의 수) </br> - 1초 마다 load_avg값을 다시 계산 </br> - 실수값
|

또한 PintOS는 부동소수점에 대한 계산을 지원하지 않기 때문에, recent_cpu, load_avg, priority등과 같은 계산식을 처리하기 위해서 아래와 같이 fixed-point에 대한 연산을 지정해주어야한다. 이는 pintOS 공식 문서를 참고하여 관련 함수를 구현해야 한다.
|||
|------|---|
|Convert n to fixed point|n * f|
|Convert x to integer (rounding toward zero)|x / f|
|Convert x to integer (rounding to nearest)|(x + f / 2) / f if x >= 0, </br>(x - f / 2) / f if x <= 0.|
|Add x and y|x + y|
|Add x and n|x + n * f|
|Subtract y from x|x - y|
|Subtract n from x|x - n * f|
|Multiply x by y|((int64_t) x) * y / f|
|Multiply x by n|x * n|
|Divide x by y|((int64_t) x) * y / f|
|Divide x by n|x / n|

## **Brief Algorithm**

nice, recent_cpu값에 대한 변수를 thread에 추가하고, 전체 전역 변수로 load_avg를 추가한다. 또한, implement되지 않은 thread_get_nice, thread_get_recent_cpu, thread_get_load_avg, thread_set_nice에 대한 함수를 작성한다.<br>
mlfqs mode에서는 donation이 일어나지 않기 때문에 lock_acquire과 lock_release함수에서 해당 부분에 대해 고려하여 수정해야하고, priority set도 임의로 실행되지 않도록 수정해야한다.

## **Implementation**

- 먼저 부동소수점 연산과 관련된 함수를 아래와 같이 fixed-point.h에 구현하였다.
  
    ```cpp
    // threads/fixed-point.h

    #include <stdint.h>
    #ifndef FIXED_POINT_H
    #define FIXED_POINT_H
    #define F (1 << 14)

    int fp_convert_N_to_fp(int N) { return N*F; }
    int fp_convert_X_to_integer_zero(int X) { return X/F; }
    int fp_convert_X_to_integer_round(int X) { 
        return (X>=0)?(X+F/2)/F:(X-F/2)/F; 
    }

    int fp_add_X_and_Y(int X, int Y) { return X+Y; }
    int fp_sub_Y_from_X(int X, int Y) { return X-Y; }
    int fp_add_X_and_N(int X, int N) { return X+N*F; }
    int fp_sub_N_from_X(int X, int N) { return X-N*F; }
    int fp_mul_X_by_Y(int X, int Y) { return ((int64_t)X)*Y/F; }
    int fp_mul_X_by_N(int X, int N) { return X*N; }
    int fp_div_X_by_Y(int X, int Y) { return ((int64_t)X)*F/Y;}
    int fp_div_X_by_N(int X, int N) { return X/N; }

    #endif
    ```

- nice, recent_cpu값에 대한 변수를 thread에 추가하고, 초기화해주었다.  

    ```cpp
    // threads/thread.h

    struct thread
    {
        ...
        /*Variable for Advanced Scheduler*/
        int nice;
        int recent_cpu;
        ...
    };
    ```

    ```cpp
    // threads/thread.h

    static void
    init_thread (struct thread *t, const char *name, int priority)
    {
        ...
        t->nice = 0;
        t->recent_cpu = 0;
        ...
    }
    ```

- 전체 전역 변수로 load_avg를 추가하고, 초기화하였다.

    ```cpp
    //threads/thread.c
    ...
    int thread_load_avg;
    ...
    void
    thread_start (void) 
    {
        ...
        /* Initialize Load Avg */
        thread_load_avg = 0;
    }
    ```

- mlfqs mode에서 priority를 결정하기 위한 변수(recent_cpu, load_avg)들의 계산식을 구현하였다.

    ```cpp
    // threads/thread.c

    void mlfqs_cal_priority(struct thread *thrd){
        //priority = PRI_MAX - (recent_cpu/4) - (nice*2)
        if(thrd != idle_thread) {
            thrd->priority = fp_sub_Y_from_X(PRI_MAX, fp_add_X_and_Y(
            fp_convert_X_to_integer_round(fp_div_X_by_N(thrd->recent_cpu, 4)), 
            fp_mul_X_by_N(thrd->nice, 2)));
        }
    }

    void mlfqs_cal_recent_cpu(struct thread *thrd){
        //recent_cpu = (2 * load_avg) / (2 * load_avg + 1) * recent_cpu + nice
        if(thrd != idle_thread) {
            thrd->recent_cpu = fp_add_X_and_N(fp_mul_X_by_Y(
            fp_div_X_by_Y( fp_mul_X_by_N(thread_load_avg, 2),
            fp_add_X_and_N(fp_mul_X_by_N(thread_load_avg, 2), 1)),
            thrd->recent_cpu) , thrd->nice);
        }
    }

    void mlfqs_inc_recent_cpu(){
        // 1 tick 마다 running thread의 recent_cpu이 1씩 증가
        if(thread_current() != idle_thread){
            thread_current()->recent_cpu = 
            fp_add_X_and_N(thread_current()->recent_cpu, 1);
        }
    }

    void mlfqs_priority(){
        //4 tick 마다 모든 thread의 priority를 다시 계산
        struct list_elem *e;
        struct thread *thrd;
        for(e = list_begin(&all_list); e != list_end(&all_list); e = list_next(e))
        {
            thrd = list_entry(e, struct thread, allelem);
            mlfqs_cal_priority(thrd);
        }
    }

    void mlfqs_recent_cpu(){
        //1초 마다 모든 thread의 recent_cpu를 다시 계산
        struct list_elem *e;
        struct thread *thrd;
        for(e = list_begin(&all_list); e != list_end(&all_list); e = list_next(e))
        {
            thrd = list_entry(e, struct thread, allelem);
            mlfqs_cal_recent_cpu(thrd);
        }
    }

    void mlfqs_load_avg(){
        //load_avg = (59/60) * load_avg + (1/60) * ready_threads
        if(thread_current() != idle_thread) {
            thread_load_avg = fp_add_X_and_Y(
            fp_mul_X_by_Y(fp_div_X_by_Y(
                fp_convert_N_to_fp(59), fp_convert_N_to_fp(60)), 
                thread_load_avg), 
            fp_mul_X_by_N(fp_div_X_by_Y( 
                fp_convert_N_to_fp(1), fp_convert_N_to_fp(60)),
                fp_add_X_and_Y(list_size(&ready_list) , 1)));
        } else {
            thread_load_avg = fp_add_X_and_Y(
                fp_mul_X_by_Y(fp_div_X_by_Y( 
                    fp_convert_N_to_fp(59), fp_convert_N_to_fp(60)), 
                thread_load_avg), 
                fp_mul_X_by_N(fp_div_X_by_Y( 
                    fp_convert_N_to_fp(1), fp_convert_N_to_fp(60)), 
                list_size(&ready_list)));
        }
    }
    ```

- nice를 set/get하는 함수(thread_set_nice, thread_get_nice), recent_cpu, load_avg get하는 함수를 구현하였다.

    ```cpp
    // threads/thread.c

    /* Sets the current thread's nice value to NICE. */
    void
    thread_set_nice (int nice) 
    {
        //현재 thread의 nice를 input한 nice로 set.
        thread_current()->nice = nice;
    }

    /* Returns the current thread's nice value. */
    int
    thread_get_nice (void) 
    {
        //현재 thread의 nice를 return
        return thread_current()->nice;
    }

    /* Returns 100 times the system load average. */
    int
    thread_get_load_avg (void) 
    {
        //현재 system의 load_avg를 100배한 값을 부동소수점 연산으로 계산하고 return
        return fp_convert_X_to_integer_round(fp_mul_X_by_N(thread_load_avg, 100));
    }

    /* Returns 100 times the current thread's recent_cpu value. */
    int
    thread_get_recent_cpu (void) 
    {
        //현재 thread의 recent_cpu를 100배한 값을 부동소수점 연산으로 계산하고 return
        return fp_convert_X_to_integer_round(fp_mul_X_by_N(thread_current()->recent_cpu, 100));
    }
    ```

- 일정 시간마다 다시 계산해야하는 값에 대한 함수들을 호출하도록 구현하였다.

    ```cpp
    // threads/thread.c

    /* Timer interrupt handler. */
    static void
    timer_interrupt (struct intr_frame *args UNUSED)
    {
        ticks++; //Since OS booting.
        thread_tick ();

        if(!thread_mlfqs){
            thread_wakeup(ticks);   //OS BOOT이후 TICKS와 비교
        } else {
            // 1 tick 마다 running thread의 recent_cpu이 1씩 증가
            mlfqs_inc_recent_cpu();  

            if(!(ticks % TIMER_SLICE)){
                //4 tick 마다 모든 thread의 priority를 다시 계산
                mlfqs_priority(); 

                if(!(ticks % TIMER_FREQ)){
                    //1초 마다 모든 thread의 recent_cpu와 load_avg를 다시 계산
                    mlfqs_load_avg();
                    mlfqs_recent_cpu();
                }
            }
            thread_wakeup(ticks);
        }
    }
    ```

- mlfqs mode에서 lock_acquire과 lock_release에 donation과 관련된 부분을 제거하고, priority set도 임의로 실행되지 않도록 수정하였다.

    ```cpp
    // threads/synch.c
    void lock_acquire(struct lock *lock)
    {
        ...
        if (!thread_mlfqs) {
            if (lock->holder) {
                thrd_cur->wait_lock = lock;
                list_insert_ordered(&lock->holder->donation_list, &thrd_cur->donation_elem, thread_comparepriority, NULL);
                donate_priority(thrd_cur);
            };
        }
        ...
        if (!thread_mlfqs)
            thrd_cur->wait_lock = NULL;
    }

    void lock_release(struct lock *lock)
    {
        ...
        if (!thread_mlfqs) {
            lock_remove(lock);
            reset_priority(lock->holder);
        }
        ...
    }
    ```

    ```cpp
    // threads/thread.c

    /* Sets the current thread's priority to NEW_PRIORITY. */
    void
    thread_set_priority (int new_priority) 
    {
        struct thread *thrd_cur = thread_current();
        if(!thread_mlfqs){      
            //mlfqs모드에서는 아래의 과정이 필요하지 않다.
            thrd_cur->origin_priority = new_priority;
            reset_priority(thrd_cur);
            thread_compare(); 
        }
    }
    ```

<br><br>

# **Discussion**
## 1. MLFQ Nice Test Fail Issue
> 위 Test를 진행하던 도중, 개인 Vmware에서는 통과가 되었지만 서버에서는 통과가 되지 않는 문제가 발생하였다. 원인을 분석한 결과, 첫번째 threads가 지정된 시간보다 훨씬 CPU를 점유하고 있다는 것으로 파악되었다. 코드를 분석해보았을 때, mlfqs모드가 아닌 일반모드에서는 nice에 의해 priority가 변하지 않기 때문에 관련이 없지만, mlfqs모드에서는 nice에 의해 priority가 변하기 때문에 이를 실시간으로 update 시켜야 할 필요가 있다고 생각했다. 다음과 같은 과정으로 코드를 수정하였다. 
> Nice를 계산하여 priority를 계산하는 method에 priority를 sorting해주는 코드를 작성한다. 이유는 priority를 계산하는 즉시 ready_list에 있는 thread들의 priority 순서를 update 해주어야 한다고 생각했다. 그 결과, thread들이 점유하고 있어야 하는 시간보다 훨씬 적게 CPU를 점유하는 상황이 발생하였다. 이후, 이 nice와 recent_cpu는 짧은 시간마다 계속 갱신되는 것이기 때문에, 위와 같은 상황이 발생할 수 있다고 생각하였고 자고 있는 thread를 깨우기 직전에 sorting을 진행하여 그 sorted된 값으로 scheduling이 이루어져도 무방하고, thread들은 점유하고 있어야 할 시간만큼 점유할 수 있을 것이라 생각했다. 그에 따라, thread_wakeup함수에 list_sort(&ready_list,thread_comparepriority,NULL)을 추가하였고 의미있는 결과를 얻었다. 이전에는 thread가 점유하는 tick이 714, 709에서만 번갈아 나오다가, 690대까지 점유하는 결과를 얻었다.
> //하지만, 매 실행마다 결과가 크게 3가지로 나누어지는 것을 볼 수 있었고, 문제는 완전히 해결되지 않았다.
문제가 해결되지 않아 PintOS 공식 문서의 FAQ부분을 읽어본 결과, TEST가 통과하지 않는다면 Timer Interrupt Handler 쪽을 살펴보라는 조언을 얻었다.

```cpp
static void timer_interrupt (struct intr_frame *args UNUSED)
{
    ticks++; //Since OS booting.
    thread_tick ();

    if(!thread_mlfqs){
        thread_wakeup(ticks);//OS BOOT이후 TICKS와 비교
    } else {
        mlfqs_inc_recent_cpu();
        if(!(ticks % TIMER_SLICE)){
            mlfqs_priority();
            if(!(ticks % TIMER_FREQ)){
                mlfqs_recent_cpu();
                mlfqs_load_avg();
            }
        }
        thread_wakeup(ticks);
    }
}   
```

> 위 코드가 수정 전의 timer interrupt method이다. Recent_cpu와 load_avg의 call 순서를 보면 막연히 공식 문서에 나온 순서대로 call을 한 것을 볼 수 있다. 그렇지만, 각각의 값에 대한 계산식을 생각해보면 load_avg를 먼저 call하여 계산을 진행하고 recent_cpu를 호출해야 올바르다고 할 수 있다. 그 이유는 recent_cpu의 계산값이 load_avg에 영향을 받기 때문이다. 반면, load_avg는 그렇지 않다. 이에 따라 다음과 같이 코드를 수정하였다. 그 결과 문제가 모두 해결되었다.

```cpp
if(!(ticks % TIMER_FREQ)){
    mlfqs_load_avg();  
    mlfqs_recent_cpu();          
}
```

## 2. Prioirty vs. Origin_Priority Issue on Priority Donation
> Priority donation을 구현할 때 처음 작성시에는 set priority부분에서 priority에 새로운 값을 대입하였다. 구현하고자 하는 방향에는 origin_priority와 priority를 구분하였는데, priority를 reset하거나 set할 때 priority에 대입을 하다보니 priority donation 중에 일어난 priority와 값이 구분되지 않아 본래의 priority로 돌아가고자 할 때 문제가 생기는 현상이 발생하였다. 이에 대한 해결 방법으로, set 또는 reset 하는 priority는 origin_priority에 대입하고, 그 외 priority donation 등 좀더 포괄적으로 많은 기능을 수행하는 priority는 priority 변수에 대입하였다. 그 결과 문제는 해결되었다.

## 3. What We have Learned
> 이번 과제를 통해 thread들이 어떻게 제어되는지, 공유 자원에 대해서 어떻게 관리되어야 하고 그 방법은 무엇들이 있는지, 처음에 고안한 thread scheduling 정책 이외에 다른 정책으로는 어떠한 것들이 있는지에 대해 배우고 이를 코드로 구현해보았다. 구현 중 어려웠던 부분은 time 문제라고 생각한다. time을 thread들이 동시에 사용하기 때문에, 어떠한 작업이 진행되고 있을 때 다른 thread에서 일어나는 일들을 고려해야 하고, 그 일들로 인해 현재 running thread에게는 어떤 영향을 미치는지, 또한 예정되어 있던 일들이 어떻게 수정되어야 하는지를 고려하는 것이 상당히 까다로웠다. Discussion 1번도 같은 맥락이라고 생각한다. 결국 이 문제도 thread들이 CPU를 점유하고 있어야 할 Time이 틀렸기 때문에 발생했다고 생각한다. Operating System을 구현 할 때 Concurrency Issue가 상당히 중요하고 까다로운 문제라는 것을 배웠다.

<br>

# **Result**
서버에서 make check를 입력하여 나온 모든 test에 대한 결과값이다.

![Result](./img/ResultScreenShot.png)

위와 같이 이번 PintOs Project 1의 모든 test를 pass한 모습을 볼 수 있다.
