Final Report for Project 1: User Programs
===================================

# Changes Since Initial Design Document

## Part 1: Argument Passing
We had to parse the string in `process_execute` before `thread_create`
was called so that it would obtain the correct value for its first argument,
which is supposed to only be the name of the program. We also made sure to push
the arguments onto the stack inside `process_execute` after `load` was called
so that the stack could be set up first. Then we followed our
design document and pushed the arguments onto the stack as was specified
in the project specifications. Additionally, in our design doc, we had a stack
pointer `int **esp` in `start_process` that we changed to a `char ** esp`.

## Part 2: Process Control Syscalls
We had to modify the child_info struct so that it looks like

```
struct child_info
  {
    /* Used to pass info between parent and child thread */
    struct semaphore *wait_semaphore;
    struct semaphore *wait_child_exec;   /* A reference to the semaphore used to release parent from
                                            waiting for child to finish loading */
    int *process_loaded;
    int exit_code;
    pid_t pid;
    struct list_elem elem;
    int counter;
  };
```
The wait_semaphore is used by a parent to wait for a child and the
wait_child_exec member is used to wait for a child to finish loading. The
process_loaded member is how the child tells the parent whether it successfully
loaded or not. Thus, using wait_child_exec and process_loaded, we can force
the parent process to wait for its child to finish loading and return -1 if it
failed. For exec, we have to make sure that the string is inside the user's
virtual address space and that it has been instantiated, or else we terminate
the parent process. We also initialize exit codes to be -1. Since all programs
that exit successfully will use the exit() syscall, we change the exit code
when handling that syscall in syscall_handler.

## Part 3: File Operation Syscalls
We changed our implementation of the fd_table to be a member in the thread
struct, since each process can have up to 128 files open. We have dummy values
for the first two indices to represent stdin and stdout. We also keep track
of `latest_fd` inside the thread struct so we know where to start finding
the next available file descriptor from. The fd_table also points to file
pointers rather than the files themselves since it is easier to deal with
the pointers. We also place a file pointer inside the thread struct
that points to its own executable, so that it can lock that file and release
it when it is about to exit. This allows us to prevent the executable
from being written while it is being run. Many of these syscalls can have
invalid addresses being passed to it so we also have to check them before
we execute them. All of this is checked and handled in the syscall handler.
For **open** and **close** syscalls, we must also add or remove the file object
to the running thread's fd_table. This can be done through the
`insert_file_to_fd_table` and `close_file` functions that we defined in thread.c
which automatically gets the next available fd and inserts it or removes it
(this automatically wraps around to fd 2 if we reach 128, so we can reuse fds).
Since we accept a string in some of the syscalls, like in exec, we also have
to check if the top and bottom of the passed in buffer are valid user addresses
(not unmapped, all of it fits in the user virtual address space, and not NULL).
When we exit a thread, we also make sure to close all of its open files.

# Project Reflection

## Work Distribution

#### Aaron Xu
Aaron wrote the portion of the design document for tasks 1 and 3 and
completed the debugging questions. For the coding part, he was instrumental in
the implementation of the code for tasks 1, 2, 3 and was the most productive in
debugging failed test cases as well. Aaron also wrote everything in the final
report in the section detailing the changes since the design document.

#### Aidan Meredith
Aidan helped complete the debugging questions. For the coding part, he committed
a lot of work and effort into debugging failing test cases.

#### Louise Feng
Louise wrote the portion of the design document for task 2. She also wrote the
responses to the additional questions. For the coding part, she helped write the
2 additional test cases, worked a bit on implementing task 1, and helped debug
failing test cases for each task. Louise also wrote everything in the final
report in the project reflection section.

#### Yena Kim
Yena wrote the portion of the design document for task 2. She also wrote the
responses for the additional questions. For the coding part, she helped write
the 2 additional test cases and successfully debugged many failing test cases.
Yena also wrote everything in the final report for the student testing report.

## Things That Went Well
A major improvement for our group over project one was our understanding of
and effort put into the design document. We wrote exhaustive documentation after
considering everything needed to be done for each task and did our best to be
as specific as possible. We worked together to understand possible faults in
our design decisions and arrived at the implementation we ultimately decided on
through precise contemplation and discussion. The design document was completed
with everyone present so each group member was able to contribute their ideas,
allowing for productive discussion and let us consider each task from multiple
different perspectives to arrive at a comprehensive solution. Also, when
everyone got together to work, we were very productive and worked well together,
helping each other understand the code and the test cases. Everyone worked on
every piece of code, either through writing it or debugging. Overall, the task
that went the smoothest for us was task 1, which required the least amount of
debugging from us after going with our initial implementation plan.

## Things That Could've Been Improved
Time management did not go so great for us throughout this project. While we
finished our design document in a very timely manner, the actual coding part was
poorly planned and we weren't able to finish and pass the last few tests until
after the deadline. For the duration of the time allotted for coding, our
communication was a bit fractured. One of our teammates also had to be out of
town for nearly a week and rather than allocating work for everyone to do and
communicating throughout, we mostly waited until she came back before we started
working together as a group and actually coded (with the exception of Aaron who
did start working and completed a good amount of work himself). We definitely
will better allocate our time for the next project and communicate better between
our group to plan the timeline of what we need to get done in order to not have
to frantically scramble to finish near the deadline. In terms of technical
difficulties, we had a hard time passing the `multi-oom` test, `syn-write`, and
`syn-read`. A method that may have alleviated the problems we faced that we
didn't pay too much regard to is wholly reading and understanding each of the
tests before starting to code. While a few of the group did look at the tests
beforehand, we only did a cursory glance at them instead of making sure we had
a full and comprehensive understanding of them. If we had done that we may have
been able to originally write code that wouldn't run into the obstructions that
the tests test against.


# Student Testing Report

## Test 1: sc-null-ptr

#### Description
This test case tests that the system will properly deal with a NULL stack 
pointer.

#### Overview
We set the stack pointer to a NULL pointer. The process must then be terminated 
with an exit code of -1.

#### Output
sc-null-ptr.output:
```
Copying tests/userprog/sc-null-ptr to scratch partition...
squish-pty bochs -q
========================================================================
                       Bochs x86 Emulator 2.6.7
              Built from SVN snapshot on November 2, 2014
                  Compiled on Oct  1 2017 at 07:27:32
========================================================================
PiLo hda1
Loading..........
Kernel command line: -q -f extract run sc-null-ptr
Pintos booting with 4,096 kB RAM...
383 pages available in kernel pool.
383 pages available in user pool.
Calibrating timer...  204,600 loops/s.
hda: 5,040 sectors (2 MB), model "BXHD00011", serial "Generic 1234"
hda1: 175 sectors (87 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 101 sectors (50 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'sc-null-ptr' into the file system...
Erasing ustar archive...
Executing 'sc-null-ptr':
(sc-null-ptr) begin
sc-null-ptr: exit(-1)
Execution of 'sc-null-ptr' complete.
Timer: 255 ticks
Thread: 46 idle ticks, 187 kernel ticks, 24 user ticks
hda2 (filesys): 61 reads, 206 writes
hda3 (scratch): 100 reads, 2 writes
Console: 883 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off..
```
sc-null-ptr.result:

`PASS`

#### Kernel Bugs
Right now, the exit code is changed to -1 when the page fault occurs in the method `page_fault`. However, if the kernel changed the exit code to -1 elsewhere, the test case would output an exit code of 0 instead.

If we did not print out the exit code where we did in `thread_exit`, the newly changed exit code of -1 would've been lost since the struct where we store it could've been freed.

## Test 2: test-tell

#### Description
This test case tests that the file system call `tell` works properly.

#### Overview
We create and open a new file. Calling `tell` returns the position of the next 
byte to be read or written. This test calls `tell` when a file is opened for 
the first time, so the next byte to be read should be at the beginning of the 
file. Therefore calling `tell` should return 0, which denotes the beginning of 
the file.

#### Output
test-tell.output:
```
Copying tests/filesys/base/test-tell to scratch partition...
squish-pty bochs -q
========================================================================
                       Bochs x86 Emulator 2.6.7
              Built from SVN snapshot on November 2, 2014
                  Compiled on Oct  1 2017 at 07:27:32
========================================================================
PiLo hda1
Loading..........
Kernel command line: -q -f extract run test-tell
Pintos booting with 4,096 kB RAM...
383 pages available in kernel pool.
383 pages available in user pool.
Calibrating timer...  204,600 loops/s.
hda: 5,040 sectors (2 MB), model "BXHD00011", serial "Generic 1234"
hda1: 175 sectors (87 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 107 sectors (53 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'test-tell' into the file system...
Erasing ustar archive...
Executing 'test-tell':
(test-tell) begin
(test-tell) create "test_file"
(test-tell) open "test_file"
(test-tell) file position is correct
(test-tell) end
test-tell: exit(0)
Execution of 'test-tell' complete.
Timer: 287 ticks
Thread: 48 idle ticks, 191 kernel ticks, 50 user ticks
hda2 (filesys): 89 reads, 223 writes
hda3 (scratch): 106 reads, 2 writes
Console: 983 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off..
```
test-tell.result:

`PASS`

#### Kernel Bugs
If creating a file did not work, then the test case would not have passed since there was no file to call `tell` on.

If the test case did not involve calling `CHECK` on `create` and `open`, we might not have been able to create or open a file to call `tell` on, making the test case fail.

## Experience
There are a lot of test cases already implemented for Pintos, so it took us 
some time to think of situations that have not yet been covered. In order to 
figure out what test cases to make, we had to read through all the test cases 
and understand exactly what was already being tested. This allowed us to have a 
deeper understanding of how all the functions are used and what their expected 
outputs are. The Pintos testing system is too slow and not always very 
informative about the results if a test fails. Many of the times that we 
encounter page faults or kernel panics, we don't know why. The failure messages 
from the tests do not help, and we have to spend a lot of time debugging our code 
without an idea of where it might erroring.To write our test cases, we had to 
learn how tests work in Pintos, so we got familiar with functions such as `check` 
and `fail`. These functions allow us to understand the different steps that we 
need to take in our tests and figure out which parts have failed if any. We also 
couldn't test just one function. There are so many parts to Pintos, and we had to 
figure out how they all relate to each other. So for the test to pass, we have to 
make sure each part of the pipeline is correctly working. For example, to test 
`tell`, we had to create and open a file before actually calling `tell`. And in 
order for anything to print, we also have to implement the other system calls as 
well. This means that we have to make sure that all these parts are working 
correctly just to test `tell`. 
