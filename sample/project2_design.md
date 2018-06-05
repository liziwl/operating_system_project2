Design Document for Project 2: User Programs
============================================

## Group Members


* Louise Feng <louise.feng@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aaron Xu <aaxu@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>

## Task 1: Argument Passing
### Data Structures and Functions
We will be modifying `start_process` so that it parses the arguments passed into
**file_name**, where the first space delimited element is the actual file name
and the rest of the arguments are arguments passed to that program. Currently,
when the function calls `load`, it passes in the string containing both the
file name and the arguments. We will change it so that it passes in only the
file name.

```
static void
start_process (void *file_name_)
{
	// Count the number of arguments passed in
	int num_args = 1;
	char *ptr;
	char *arg = strtok_r(file_name, " ", &ptr);
	while (arg = strtok_r(NULL, " ", &ptr)) {
	num_args += 1;
	}
	// Parse the args
	int i = 0;
	char *args[num_args];
	for (arg = strtok_r(file_name, " ", &ptr); arg != NULL; arg = strtok_r(NULL, " ", &ptr)) {
	args[i] = arg;
	i++;
	}
	// Set temp user stack pointer
	int **esp = (int **) malloc(4);
	*esp = PHYS_BASE;
	/* Store arguments in thread stack */
	for (i = num_args - 1; i >= 0; i--) {
	  *esp -= strlen(*(args + i)) + 1;
	  strlcpy(*((char **)esp), *(args + i), strlen(*(args + i)) + 1);
	}
	// word align the stack pointer
	int offset_to_word_align = ((uintptr_t) *esp) % 4;
	int j;
	for (j = 0; j < offset_to_word_align; j++) {
	  *esp -= 1;
	  **((char **)esp) = 0;
	}
	*esp -= 4;
	**((char **)esp) = NULL;
	// Store addresses of the args in the thread stack
	uintptr_t addr = PHYS_BASE;
	for (i = num_args - 1; i >= 0; i--) {
	  *esp -= 4;
	  addr -= strlen(*(args + i)) + 1;
	  *((char **)(*esp)) = (char *)addr;
	}
	*esp -= 4;
	*((char **)(*esp)) = *esp + 4;
	*esp -= sizeof(int);
	*((char **)(*esp)) = num_args;
	*esp -= 4;
	*((char **)(*esp)) = NULL;

	free(esp);

	...

	success = load (args[0], &if_.eip, &if_.esp);
	...
}
```

### Algorithms
In `start_process`, we will split the variable **file_name** by spaces and obtain
a new **file_name** variable and an **args** variable. We push all the arguments
in **args** onto the stack, starting at PHYS_BASE in the order specified by the
specs (right to left). In order to do this, we will copy each string in **args**
into the the stack pointer, since the memory address has to be below PHYS_BASE.
This will prevent a page fault from occuring. Then, we can pass in the new
**file_name** variable into `load` so that it opens the correct file. After
`load` sets up the stack and initializes the stack pointer to PHYS_BASE,
we can move it to the end of our pushed arguments (the one with the lowest
address value) and that should set up the user stack correctly.

### Synchronization
Since each process can only access its own stack, there is no need for
synchronization. The thread does not wait for any resources and no other threads
can access this thread's resources, so there are no race conditions.

### Rationale
We implemented the copying of the passed in arguments from the kernel stack
to the user stack in `start_process` because it is at the beginning of a thread's
life cycle. It also calls `load` which uses **file_name**, so we have to make
sure that we are passing in the correct value. Thus, in `start_process`, we
parse the arguments, push the arguments onto the stack, and then call `load`
with the correct arguments. This works because even though the user stack
has not yet been set up (which is done in `load`), we know that it will start
at PHYS_BASE, so we can set up the values beforehand. After setting up the
user stack, we can simply move the pointer to the end of the arguments we
previously set up.


## Task 2: Process Control Syscalls

### Data Structures and Functions

We create a struct `child_info` in `thread.h` for every child thread created. The parent of the child will have a list of `child_info` structs so that we can access the information about the child that we need. The list for every parent will be as long as the number of children it has. Inside `child_info` we will keep track of a semaphore shared between the parent and child as well as the exit code of the child, the child's pid, a list elem, and a counter denoting if the parent and the child are running:

```
struct child_info
	{
		struct semaphore shared_sema;
		int exit_code;
		pid_t child_pid;
		struct list_elem elem;
		int counter;
	}
```

We add a pointer to this created struct (`child_info`) to the child, and add it to the list of `child_info` structs that the parent has. In `thread.h` we modify the thread struct to include the following:

```
struct thread
	{
		...
		struct child_info *info;
		struct list children;
	}
```

We also modify `syscall_handler` to implement the handling of each of the syscalls we add the functionality for in this section: halt, exec, wait, and practice. Each will be handled in a new `else if` clause. We also need to ensure that the stack pointer is valid in order to proceed with a syscall. If the stack pointer isn't valid, we must return -1 instead.

```
static void
syscall_handler (struct intr_frame *f UNUSED)
{
	if (**esp == null) {
		return -1;
	}
	... \\keep the code already written as is
	else if (args[0] == SYS_HALT)
		{
			...
		} else if (args[0] == SYS_EXEC)
		{
			...
		} else if (args[0] == SYS_WAIT)
		{
			...
		} else if (args[0] == SYS_PRACTICE)
		{
			...
		}

}
```

In `thread.c` we modify the exit code of the thread that is exiting in `thread_exit` so that the exit code is -1 when the process has been terminated by the kernel, 0 otherwise. We do this by setting the exit code to be -1 at the start of the method. We change it to be 0 if the process is a user process, which means it exits by itself and not through the kernel.

```
void
thread_exit (void)
	{
		ASSERT (!intr_context ());

		thread_current ()->info->exit_code = -1;

		#ifdef USERPROG
			thread_current ()->info->exit_code = 0;
			process_exit ();
		#endif
		...
	}
```


### Algorithms

For each of the system calls, we add another `else if` clause to `syscall_handler` in order to check if that particular syscall is being called. We also want to make sure that a syscall will return -1 in the case that the stack pointer isn't valid (instead of erroring). We can do this by checking if `**esp == null` in `syscall_handler`. If the condition is true, then we simply return -1. Otherwise, we continue on and handle the syscalls as expected. For `SYS_PRACTICE`, we simply grab the first argument from the stack and increment it by one. For `SYS_HALT` we terminate Pintos by calling `shutdown_power_off()`. For `SYS_EXEC`, we want to run the executable that is in `args[1]`. However, we need to know if the executable has been successfully loaded or not before we can continue. To ensure this, we pass in a semaphore into `process_execute` by concatenating it to the string `args[1]`. This semaphore is initialized at 0 and will have `sema_up` called on it only when `process_execute` is completely done running.

For `SYS_WAIT`, we want to wait for the pid that is specified in `args[1]` to finish executing. First we check that the given pid is actually a child of the currently running process, by iterating through the list of `child_info` structs that are its children and checking `child_pid` held by every struct. If it doesn't find one that is equivalent to the argument for `process_wait` we simply return -1. We also return -1 if the child process was terminated by the kernel instead of calling `exit()`. We do this by changing the exit code in `thread_exit()` in `thread.c`.

We create an instance of the struct `child_info` in `thread_create ()` so that every child created has a `child_info` associated with it by allocating space for it and initializing it. We set that child's instance variable `info` to be a pointer to that newly created `child_info` instance. We also add this struct to the parent's list of `child_info` structs by calling `thread_current ()` to get the parent. We also initialize the semaphore inside `child_info` to be 0 at this time. This semaphore is accessible to both parent and child. It will always have `sema_up` called on it when the child process is finished running. If we call the syscall wait, this will make the process go to `process_wait`, where it will try to call `sema_down` on the shared semaphore. If the child process is finished running, and therefore incremented the value of the semaphore, then the parent process can continue. Otherwise, it will wait until the semaphore is incremented when the child process finishes.

We want to free the memory we allocated for the `child_info` struct when both the parent and the child have exited and the struct is no longer needed. For this we keep a counter `counter` initialized to 2 in each `child_info`. When we exit a parent thread(`thread_exit()`), we make sure to go through `children`, the list of `child_info` structs that correspond to its children, and decrement the counter. We also decrement counter when the child process terminates. If both processes have terminated, then the struct can be freed so after we decrement the counter, we check if we should free the struct and if `counter == 0` then we do. We do this before `process_exit` is called in `thread_exit()`

Before we utilize the stack pointer, we have to make sure it is in valid memory space. When we call `args[0]` or `args[1]` we first have to check that the whole component that the pointer is pointing at is in valid memory. We do this by calling `is_user_vaddr` on the virtual address of the stack pointer.

### Synchronization

To implement the syscall wait, we create a shared struct (`child_info`). Even though it is shared, we do not have to worry about synchronization issues, because only one thread will try to write to that struct.

For the syscall exec, we use a semaphore to ensure that the parent knows whether or not the child has successfully loaded its executable.

### Rationale

We initially tried to change the exit code in the `kill` function in `exception.c`, but we decided to instead modify the exit code in `thread_exit ()` in order to have more clear and concise code. One aspect of our implementation that may not be optimal is that when we check that the thread a parent is waiting for is actually a child of the parent, we iterate through a list of the parents and exhaustively compare the pid of the child with each pid of every child of the parent until we find one that matches (or until we know that none do) where there may be a faster way to check (maybe if we implemented a set or something). In other respects, our code and logic seem to the point and get the job done in a reasonable manner.


## Task 3: File Operation Syscalls
### Data Structures and Functions
In thread.c, we can define a global variable `struct lock filesys_lock` that
will allow us to restrict file operation syscalls to only call one filesystem
function at a time. Then in `thread_init`, we call lock_init(&filesys_lock) to
initialize the lock.

We will need an array of 128 file objects and declare this in `thread_init`.  
`struct file files[128]; // Global variable in thread.c`  
`files = malloc(128 * sizeof(struct file)); // Initialized in thead_init`  
The file descriptor will be the index to the array and accessing this array
will return the corresponding file object.

We will also need a global variable to keep track of the most recent file
descriptor assigned.  
`int next_fd = 3 // Goes in thread_init`

### Algorithms
In `file`, before returning the file descriptor, we will get the file object
by calling `filesys_open` and store this into `files[next_fd]` and then
increment `next_fd` by one.

In order to prevent executable files that are running from being written to,
we must remove the line `file_close (file);` from `load()`. This is because
`file_close` calls `file_allow_write` and we do not want this to happen.
Instead, we will call `file_deny_write` in `load` so that we will be able
to prevent writes to the executable file.

In `write`, given the file descriptor, we can access the file object by indexing
the `files` array. We can check the **deny_write** status of the file object
and return -1 if the status is True.

In `thread_exit`, we can grab the name of the current running executable since
we stored that onto the user stack in part 1. Then we can iterate through `files`
and find the  file object with this name and get its associated file descriptor and then do
`fileObj->inode->open_cnt--;` on that returned file object. Then we can call
`file_allow_write` on the file object to allow other files to write to it.
If `open_cnt` is 0, we can free the file object by calling `file_close` on the
file object and set the value of `files` at index of the file descriptor we
obtained to NULL.


### Synchronization
For each filesystem function, before the syscall, we do
`lock_acquire(&filesys_lock);` and `lock_release(&filesys_lock)` after every
syscall. This will prevent concurrent calls to syscall from happening since
there is only one lock and all syscalls are placed in the critical section. Thus,
there will be no race conditions or multiple calls when trying to perform
a filesystem operation.

### Rationale
We need the `filesys_lock` because we want to restrict it to only have one
file operation occur at a time. We have the `files` array act as pintos's file
descriptor table, so we can access both the file descriptor and the file object.
We start `next_fd` at 3 because the first three values are reserved for stdin,
stdout, and stderr. We keep track of the open file objects so that we can
access the number of openers of files from `open` and, `write`, and `thread_exit`.
We use an array here because we know the order in which file descriptors are created
(smallest unused value) and it will be quick to index through this array.

## Design Doc Additional Questions

### 1. Please identify a test case that uses an invalid stack pointer ($esp) when making a syscall. Provide the name of the test and explain how the test works.

The sc-bad-sp test uses a stack pointer that is invalid because it points to a bad address. The test first assigns (on line 18) the stack pointer to the address `$.-(64*1024*1024)` by moving the address into the contents of `$esp`, the stack pointer. This is a bad address because it is 64MB below the code segment. After setting an invalid stack pointer, the test makes a syscall (`int  $0x30`). We do all this in assembly code by putting these instructions as arguments to asm volatile. Because the syscall was made with an invalid stack pointer, it should terminate with a -1 exit code. If this test fails (and we do not exit), then the code will not exit and will reach the next line of the test which will tell us that we failed the test and log it.

### 2. Please identify a test case that uses a valid stack pointer when making a syscall, but the stack pointer is too close to a page boundary, so some of the syscall arguments are located in invalid memory

The sc-bad-arg test uses a valid stack pointer set at the very top of the stack - the user stack ends at `0xc0000000` and we have our pointer at `0xbffffffc`, leaving 4 bytes of space left in the user stack. The next instruction (2nd instruction in the assembly code of line 14) moves `SYS_EXIT` to the location of our stack pointer `%0` corresponds to the input operand which in this case is the integer specified `SYS_EXIT`. The last instruction on the same line invokes the system call designated by our stack pointer's location, which is the `SYS_EXIT`. Since our stack pointer was at the top of the user stack with 4 bytes of it left, and `SYS_EXIT` takes 4 bytes of space, we have used all the space at the top of the user stack. We now go up the stack to look for the syscall arguments but are unable because the `SYS_EXIT` is at the very top and if we try to go further we'd be going into an invalid address. We therefore can't get the arguments and the process must be terminated with -1 exit code because the argument to the system call would be above the top of the user address space in invalid memory.


## 3. Identify one part of the project requirements which is not fully tested by the existing test suite. Explain what kind of test needs to be added to the test suite, in order to provide coverage for that part of the project.

In section 3.1.7 in the specs, it is specified that "As part of a system call, the kernel must often access memory through pointers provided by a user program. The kernel must be very careful about doing so, because the user can pass a null pointer." We don't yet have a test that checks that a syscall should fail if given a null pointer as the stack pointer. We can create a test similar to sc-bad-sp where in assembly code (using asm volatile), we assign the stack pointer to be a null pointer and then attempt to make a syscall. The correct result would be that the syscall would fail and we terminate with a -1 exit code. If the test is failed, the syscall won't exit, and we can therefore put fail ("should have called exit(-1)"); on the next line denoting that the test was failed.


## 4. GDB Questions

1. The name of the thread is **main** and the thread is at address
   **0xc000e000**. The only other thread present at this time is the **idle**
   thread at address **0xc0104000**.
       **struct thread of main**:

       ```
       0xc000e000 {tid = 1, status = THREAD_RUNNING,
       name = "main", '\000' <repeats 11 times>,
       stack = 0xc000ee0c "\210", <incomplete sequence \357>, priority = 31,
       allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020},
       elem = {prev = 0xc0034b60 <ready_list>,
       next = 0xc0034b68 <ready_list+8>},
       pagedir = 0x0, magic = 3446325067}
       ```

   	**struct thread of idle**:
	```
	0xc0104000 {tid = 2, status = THREAD_BLOCKED,
        name = "idle", '\000' <repeats 11 times>,
        stack = 0xc0104f34 "", priority = 0,
        allelem = {prev = 0xc000e020, next = 0xc0034b58 <all_list+8>},
        elem = {prev = 0xc0034b60 <ready_list>,
        next = 0xc0034b68 <ready_list+8>},
        pagedir = 0x0, magic = 3446325067}
   	```

2. **Backtrace of current thread**
	\#0  process_execute (file_name=file_name@entry=0xc0007d50 "args-none")
	at ../../userprog/process.c:32
	`process_execute (const char *file_name)`
	\#1  0xc002025e in run_task (argv=0xc0034a0c <argv+12>) at ../../thread
	s/init.c:288
	`process_wait (process_execute (task));`
	\#2  0xc00208e4 in run_actions (argv=0xc0034a0c <argv+12>) at ../../thr
	eads/init.c:340
	`a->function (argv);`
	\#3  main () at ../../threads/init.c:133
	`run_actions (argv);`

3. The name of the current thread is **args-none** with address **0xc010a000**.

	**struct threads of main**
	0xc000e000 {tid = 1, status = THREAD_BLOCKED,
	name = "main", '\000' <repeats 11 times>,
	stack = 0xc000eebc "\001", priority = 31,
	allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020},
	elem = {prev = 0xc0036554 <temporary+4>, next = 0xc003655c <temporary+12>},
	pagedir = 0x0, magic = 3446325067}

	**struct threads of idle**
	0xc0104000 {tid = 2, status = THREAD_BLOCKED,
	name = "idle", '\000' <repeats 11 times>,
	stack = 0xc0104f34 "", priority = 0,
	allelem = {prev = 0xc000e020, next = 0xc010a020},
	elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>},
	pagedir = 0x0, magic = 3446325067}

	**struct threads of args-none**
	0xc010a000 {tid = 3, status = THREAD_RUNNING,
	name = "args-none\000\000\000\000\000\000",
	stack = 0xc010afd4 "", priority = 31,
	allelem = {prev = 0xc0104020, next = 0xc0034b58 <all_list+8>},
	elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>},
	pagedir = 0x0, magic = 3446325067}

4. The thread running **start_process**, **args-none**, is created in line 45
   of **userprog/process.c**.
   `tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);`

5. **0x0804870c** caused the page fault.

6. \#0  \_start (argc=<error reading variable: can't compute CFA for this frame>,
	           argv=<error reading variable: can't compute CFA for this frame>)
	   at ../../lib/user/entry.c:9

7. The program is crashing because when we create a thread to run the user
   program, we pass in a variable to it. In the case of `args-none`,
   **args-none** is passed in as an argument to the user program. However,
   this argument has an address of 0xc0007d50. Thus, when the user program
   tries to access this argument, it will cause a page fault, since the address
   exceeds 0xC0000000 which is the upper bound for the user program virtual
   memory addresses.
