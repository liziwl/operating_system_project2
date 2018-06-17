Final Report for Project 2: User Programs
==========================================
## Group Members

* Danning XIE <11510718@mail.sustc.edu.cn>

    Task 2 implement with Ziqiang LI and Task 1 design and implement.

* Ziqiang LI <11510352@mail.sustc.edu.cn>

    Task 2 implement with Danning XIE and Task 3 design and implement, report writing.

## Task 1: Argument Passing

### Data structures and functions

### Algorithms

### Synchronization

### Rationale

## Task 2: Process Control Syscalls

### Data structures and functions


### Algorithms


### Synchronization


### Rationale

`strtok_r()` is thread-safe, `strtok()` is not. Pintos uses `strtok_r()` instead of `strtok()` so that multiple threads can call `strtok_r()` without messing up the internal state of tokens of one another.

If memory runs out during the separation of arguments, only this user program crashes in the case, but in the case where the kernel does this separation, the whole system may crash.

## Task 3: File Operation Syscalls

### Data structures and functions

#### thread.h

```c
struct thread {
    ...
    struct list opened_files;   //all the opened files
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

All of the file operations are protected by the global file system lock, which prevents doing I/O on a same fd simultaneously. We disable the interruption, when we go through `thread_current()->parent->children_list` or `thread_current()->opened_files`, to prevent unpredictable error or race condition in context switch. So they will not cause race conditions.

### Rationale

Actually, all the critical part of syscall operations are provided by `filesys/filesys.c`. At the same time, the document warns us avoiding modifying the `filesys/` directory. So the vital aspect is that poping and getting the data in the stack correctly, and be careful not to do I/O simultaneously.

## Design Changes

## Reflection

### What did each memebr do

In this project, Danning XIE is responsible for implementing Task 1 and designing as well as implementing parts of Task 2. Ziqiang LI is responsible for implementing prarts of Task 2 and designing as well as implementing Task 3.

Both members spent a great amount of time on debugging the Pintos system with `printf()`.

### What went well and wrong

