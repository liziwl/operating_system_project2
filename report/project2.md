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


## Task 3: File Operation Syscalls

### Data structures and functions

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

    Go through the process file list, and close specific process file by the file descriptor number.

- `void clean_all_files(struct list* files)`

    Go through the process file list, and close all process file. Do this when exit a process.

### Algorithms

### Synchronization

### Rationale

## Design Changes

## Reflection

### What did each memebr do

In this project, Zhihao DAI is responsible for implementing Task 1 and designing as well as implementing Task 3. Ziqiang LI is responsible for implementing Task 1 and Task 2 and designing Task 3.

Both members spent a great amount of time debugging the Pintos system with `printf()` and gdb.

### What went well and wrong

