# Final Report for Project 2: User Programs

## Group Members

- Danning XIE <11510718@mail.sustc.edu.cn>
- Ziqiang LI <11510352@mail.sustc.edu.cn>

## 1 Task 1: Argument Passing

### 1.1 Data structures and functions

#### 1.1.1 Data structures

- <process.c> 
  - New macro `# define WORD_SIZE 4`
- <thread.h>
  - Add new attributes to the `struct thread` : `struct file *self`

#### 1.1.2 Functions

The functions involved in this process is 

- <process.c> : 
  - `process_execute (const char *file_name)`
  - `load (const char *file_name, void (**eip) (void), void **esp)`
  - `setup_stack (void **esp, char * file_name)`
  - `process_wait (tid_t child_tid)`
- <syscall.c>
  - `syscall_write(struct intr_frame *f);`

Note that `process_wait (tid_t child_tid)` and `syscall_write(struct intr_frame *f);` are really task2 and task3's job. So we won't get into much details in this section.

The tokenize process of `char *` is implemented by calling `strtok_r`.

### 1.2 Algorithms

#### 1.2.1 Analysis

The forward process of how we create a new process:

- calling method <process.c> `process_execute`
- the operating system then ask for a corresponding room of memory
- create a child process of the current one by calling function <thread.c> `thread_create` and pass a function pointer of `load` to it.
- In function `load` , it loads an ELF executable from the `file_name` parameter passed in, stores the executable's entry point into `*eip` and its initial stack pointer into `*esp` by calling function `setup_stack` . 

Therefore, the main work we have to do in this task is to pass the command-line argument into the `load` and `setup_stack`, then push the arguments in correct order into the stack by operating on the stack pointer `*esp`.

#### 1.2.2 Implementation

#### 1.2.2.1 Split and Pass the arguments

The `file_name` passed into function `process_execute (const char *file_name) ` include both executable file and the argument. Therefore, the first thing we need to do is to split the executable file name and the argument.

- <process.c/process_execute> Split the thread name and save it in the variable `chr * thread_name`

  ```c
  char *save_ptr;
  thread_name = malloc(strlen(file_name)+1);
  strlcpy (thread_name, file_name, strlen(file_name)+1);
  thread_name = strtok_r (thread_name," ",&save_ptr); 
  ```

  Then create a child thread

  `tid = thread_create (thread_name, PRI_DEFAULT, start_process, fn_copy);`

- <process.c/start_process>

  pass the `file_name` into the function `load`:

  `success = load (file_name, &if_.eip, &if_.esp);`

- <process.c/load> Split the executable file name and open the file

  and then store the executable file into the current thread :

- <process.c/load> call `setup_stack` and pass the `file_name` 

- <process.c/setup_stack> In this method, we split the `argv` , count the `argc` and then push them in the correct order according to the document.

  ![](/Users/danning/Desktop/sustc/os/pro/pro2/operating_system_project2/report/stack.png)

  - First tokenize the `file_name`

  - Iterate the token and count the `argc` (argument count)

  - ask for the coresponding size of room for `argv[]`

  - push the element of argv, word align. Also make sure the `argv[argc]` is a null pointer. Push the address of `argv` and finally a return address.

    Note that we simply move the `*esp` (stack pointer) instead of pushing null value/pointer and return address.

### 1.3 Synchronization

During the function `laod`, we allocate page directory for the file, open the file, push the argument into the stack. According to **Task3**, the file operation syscalls do not call multiple filesystem functions concurrently. Therefore, we have to keep the file from modified or opened by other processes. We implement it by using `filesys_lock` (defined in `thread.h` which we will explain in **Task3**):

```c
lock_acquire(&filesys_lock);
//loading the file
lock_release(&filesys_lock);
```

Also, according to the **Task3**, while a user process is running, the operating system must ensure that nobody can modify its executable on disk. `file_deny_write(file)` denies writes to this current-running files.  

### 1.4 Rationale

In this task, we split the input arguments and pass them into function `load` to push the arguments into the stack in the corret order. We also implement lock operations to ensure that nobody can operates on the file. `file_deny_write` is also called to pass the "rox" tests.

## 2 Task 2: Process Control Syscalls

### 2.1 Data structures and functions

- <thread.h>
  - We add some new attributes to the `struct thread`:
    - `bool load_success` : whether its child process successfully loaded its executable.
    - `struct semaphore load_sema` : keep the thread waiting until it makes sure whether the child process is successfully loaded.
    - `int exit_status` 
    - `struct list children_list` : list of its child processes
    - `struct file * self` : its executable file, we've discussed it in **Task1**
    - `strut child_process *waiting_child`: a pointer to the child process it is currently waiting on 
  - We create a new struct called `child_process` and had it some attributes:
    - `tid` : its thread id. making it easy for its parent finding it.
    - `if_waited` : whether the child process has been waited. According to the document, a process may wait for any given child at most once. The attributes would be initialized to `false`
    - `int exit_status` : its exit status. used to pass to its parent process when it is wated.
    - `struct semaphore wait_sema` : This semaphore is designed for waiting process. It is used to avoid race condition

#### 2.1.2 Functions

The functions involved :

- <syscall.c> 
  - `void * is_valid_addr(const void *vaddr);`
  - `void pop_stack(int *esp, int *a, int offset)`

To implement the three system call functions the document ask for, we write a method `syscall_handler` to handle all the system calls in both **Task2** and **Task3** by checking the argument in the stack using a `switch-case` structure. The three system calls in this task are wraped into functions `syscall_halt()`, `syscall_exec()` and `syscall_wait()`, which calls the functions `shutdown_power_off()`, `process_execute()` and `process_wait()` in the process.c file.

### 2.2 Algorithms

#### 2.2.1 Analysis

In this task, we are asked to implement three kernel space system calls. A `switch-case` structure in `system_handler` let the system calls execute their coresponding code. The type of system calls read from the syscall argument located on the user process’s stack. However, we have to make sure  the process reads and writes in the user process's virtual address space. That is, we should check whether the address is pointed to a valid address before execute system calls. 

The invalid memory access include:

- NULL pointer
- Invalid pointers (which point to unmapped memory locations) 
- Pointers to the kernel’s virtual address space 

#### 2.2.2 Implementation

##### 2.2.2.1 Check valid address

We implement it in function:

```c
void *is_valid_addr(const void *vaddr)
{
	void *page_ptr = NULL;
	if (!is_user_vaddr(vaddr) || !(page_ptr = pagedir_get_page(thread_current()->pagedir, vaddr)))
	{
		exit_process(-1);
		return 0;
	}
	return page_ptr;
}
```

The function checks whether the address is valid. If the `vaddr` is `NULL` , or of the kernel address space, or points to invalid locations, `exit_process` is called to terminate the current thread with exit status -1. Otherwise it returns the pyhsical address.

##### 2.2.2.2 Pop stack

In order to pop the argument we want from the user process's stack, we realize the poping process in a method :

```c
void pop_stack(int *esp, int *a, int offset){
	int *tmp_esp = esp;
	*a = *((int *)is_valid_addr(tmp_esp + offset));
}
```

All pop operations on the stack need to call this function. It will verify if the stack pointer is a valid user-provided pointer, then dereference the pointer of a specific location(offset) which we will discuss later.

##### 2.2.2.3 Halt

We simply calls the `shutdown_power_off()` in <devices/shutdown.c> 

##### 2.2.2.1 Exec

We call <syscall.c>`syscall_exec(file_name)` first to check whether the file refered by `file_name`  is valid.

```c
pop_stack(f->esp, &file_name, 1);
if (!is_valid_addr(file_name))
	return -1;
```

If it is not, we return -1, else we call <process.c>`process_execute(file_name)`.  In function `process_execute` , we first split the thread name and create a child process with it. We wait util the `thread_create` return. Since the `tid` returned could be invalid, the parent should wait until it knows whether the child process's executable file is loaded successfully. We implement this by `thread->load_sema` which we will discuss later in the Synchronization part. We store the loading information in `thread->load_success`. If successfully loaded, the method returns its `tid`, otherwise -1.

##### 2.2.2.1 Wait

The syscall means that  the process should wait for a child process pid and retrieves the child’s exit status. First we should pop the syscall argument (child process's tid) from the user process stack and check if it is valid:

```c
pop_stack(f->esp, &child_tid, 1);
```

Then we call <process.c>`process_wait(child_tid)`. If the child process with `child_tid` exists in `thread_current()->children_list` , then we can go on wait. We implement the finding process in a method <thread.c> `find_child_proc(child_tid)` which find the corresponding `list_elem` with `tid=child_tid` in the current thread's `children_list`. 

Note that according to the document,  a process may wait for any given child at most once.  Therefore, before we step into waiting process, we should first check whether the child has been waited before. If not, set the child thread's `if_waited` to `true` and down the semaphore `wait_sema`. The `wait_sema` will be increased in method `process_exit` when child process exits. Finally we can remove the child process from `child_list` and return its `exit_status`.

### 2.3 Synchronization

#### 2.3.1 `filesys_lock` : Lock on file system operations

According to the document: the Pintos filesystem is not thread-safe. We must make sure that the file operation syscalls do not call multiple filesystem functions concurrently. And we are permitted to use a **global lock** on filesystem operations, to ensure thread safety. Therefore, we assert the global lock `filesys_lock` in `thread.h`

In this task, we use the lock in the following place:

- <process.c>`load` : we acquire the `filesys_lock` before we allocate the page directory, open executable file and all the operations we had on file. Then we release the lock at the end of the method before it returns. Note that we have to release the lock whether the loading process is successful or not.

  ```c
  bool load (const char *file_name, void (**eip) (void), void **esp){
      lock_acquire(&filesys_lock);
      ....
      // operations on the file
      if load fail:
      	goto done
      ....
      done:
      	lock_release(&filesys_lock);
  }
  ```

- <syscall.c> `exec_process` : The method opens the file with the `file_name` to check whether the file exists. Before we open, we should acquire the lock, and release it after. Note that we should release the lock whether the file exists or not.

#### 2.3.2 `thread->load_sema`

When a process is creating a new child process, it has to wait until it knows whether the child process's executable file is loaded successfully. Therefore, once the child thread is created, it downs the `wait_sema` in method <process.c>`process_execute` and block the parent thread:

```c
sema_down(&current_thread->load_sema);
```

Once the child process'c executable file finish loading, it upped the semaphore to wake up the blocked parent thread in method <process.c>`start_process`:

```c
sema_up(&current_thread->parent->load_sema);
```

Note that we should up the semaphore whether the executable file is loaded successfully or not.

#### 2.3.3 `child_process->wait_sema`

The semaphore is decreased when a thread start to wait in <process.c>`process_wait` one of its child process:

```c
sema_down(&child_process->wait_sema);
```

 Once the semaphore is decreased, the thread is blocked until the child process exits in method <thread.c>`thread_exit`. When the child process the process is waiting exits, it upped the semaphore `wait_sema` and wake up the blocked parent thread:

```c
sema_up(&thread_current()->parent->waiting_child->wait_sema);
```

### 2.4 Rationale

In this task, we accomplished three kernel system calls. To achieve that, we add some new attributes to the struct `thread` and designed a new structure `child_process` for child process. Semaphores are also used in this task to prevent race condition and make sure the execution order.

## Task 3: File Operation Syscalls

### Data structures and functions

#### thread.h

```c
struct thread {
    ...
    struct list opened_files;   //all the opened files
    int fd_count;
    ...
};
```

Holding the list of all opened files.

#### syscall.h

```c
struct process_file {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};
```

Store file pointer and file description number, as a list_elem in the `opened_files` of the `struct thread`.

#### syscall.c

- `static void syscall_handler (struct intr_frame *)`

  Handling the file syscall, going to the specific calls by the values in the stack.

- specific file syscall functions

  Call the appropriate functions in the file system library.

  ```c
  void syscall_exit(struct intr_frame *f);
  int syscall_exec(struct intr_frame *f);
  int syscall_wait(struct intr_frame *f);
  int syscall_creat(struct intr_frame *f);
  int syscall_remove(struct intr_frame *f);
  int syscall_open(struct intr_frame *f);
  int syscall_filesize(struct intr_frame *f);
  int syscall_read(struct intr_frame *f);
  int syscall_write(struct intr_frame *f);
  void syscall_seek(struct intr_frame *f);
  int syscall_tell(struct intr_frame *f);
  void syscall_close(struct intr_frame *f);
  ```

- `void * is_valid_addr(const void *vaddr)`

  Verifying `VADDR` is a user virtual address and is located in the current thread page.

- `void pop_stack(int *esp, int *a, int offset)`

  All pop operation on the stack needs to call this function. It will verify if the stack pointer is a valid user-provided pointer, then dereference the pointer.

- `int exec_process(char *file_name)`

  Sub-function invoked by `int syscall_exec()`: split string into tokens and call `process_execute()` with tokens.

- `void exit_process(int status)`

  Sub-function invoked by `int syscall_exit()`: set current thread status by `status`, and update the status of the child(current) process in its parent process. At last, call `thread_exit()`.

- `struct process_file* search_fd(struct list* files, int fd)`

  Find file descriptor and return process file struct in the process file list, if not exist return NULL.

- `void clean_single_file(struct list* files, int fd)`

  Go through the process file list, and close specific process file, and free the space by the file descriptor number.

- `void clean_all_files(struct list* files)`

  Go through the process file list, close all process file and free the space. Do this when exit a process.

### Algorithms

The Pintos filesystem is not thread-safe. File operation syscalls cannot call multiple filesystem functions concurrently. Here we add more sophisticated synchronization to the Pintos filesystem. For this project, we use a global lock defined in `threads/thread.h` on filesystem operations, to ensure thread safety. 

When a syscall is invoked, `void syscall_handler()` handle the process. All arguments are pushed in the stack when a user program doing system operation using `lib/user/syscall.c`. So, we just need to take the parameter from the stack.

1. We pop the syscall number from the stack by the `esp`.
2. we go to the specific syscall functions by the syscall number. For each file syscalls, we need to pop more detailed arguments. If the parameter we pop out is a pointer (such as a `char *`), we also need to verify its usability.
3. Each file syscalls call the corresponding functions in the file system library in `filesys/filesys.c` after acquired global file system lock.
4. The file system lock will be released anyway at last.

### Synchronization

All of the file operations are protected by the global file system lock, which prevents doing I/O on a same fd simultaneously. 

- First, we check whether the current thread is holding the global lock `filesys_lock` . If so, we release it.	

  ```c
  if (lock_held_by_current_thread(&filesys_lock))
      lock_release(&filesys_lock);
  ```

- Then we have to close all the file the current thread opens and free all its child.

  ```c
  lock_acquire(&filesys_lock);
  //close all current_thread()->opened_files
  //free current_thread()->children_list;
  lock_release(&filesys_lock);
  ```

Also, we disable the interruption, when we go through `thread_current()->parent->children_list` or `thread_current()->opened_files`, to prevent unpredictable error or race condition in context switch. So they will not cause race conditions.

### Rationale

Actually, all the critical part of syscall operations are provided by `filesys/filesys.c`. At the same time, the document warns us avoiding modifying the `filesys/` directory. So the vital aspect is that poping and getting the data in the stack correctly, and be careful not to do I/O simultaneously.

## Hack testing case

### case 1: open many files

#### Aim
This test tests the implementation of the thread can hold a large number of program files.

#### Design idea
We open 500 files on a single program. If it can survive, it passes the test case.

### case 2: Implementation of the filesize system call

#### Aim
We find that there is no test case for filesize system call, so we create one.

#### Design idea

We verify that the byte_cnt by system call is the same as the file actully is.

## Reflection

During this project, we have learned alot about user programs, semaphore, locks in operating systems: 

- How an operating system ensure the security by checking the validation of the address. An operating system is software that manages the computer hardware. The hardware must provide appropriate mechanisms to ensure the correct operation of the computer system and to prevent user programs from interfering with the proper operation of the system. 
- How a system call is implemented. Even though there is no `fork` in `pintos`, we still can do the similar thing using `process_execute`. It create a child process and allocate memory for it. It wait until it knows whether the child process is loaded successfully or not. 
- How important a semaphore or a lock can be to a operating system. semaphore blocks and wakes up the corresponding thread. A lock makes sure no other thread can touch the same resouce.

### Contributions

- Task1 :
  - Code : Danning Xie
  - Report : Danning Xie
- Task2:
  - Code : Danning Xie & Ziqiang Li
  - Report : Danning Xie
- Task3:
  - Code: Ziqiang Li & Danning Xie
  - Report: Ziqiang Li
- Hack Tests:
  - Code: Ziqiang Li
  - Report: Ziqiang Li