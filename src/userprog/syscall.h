#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "kernel/list.h"

struct proc_file {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

void syscall_init (void);
void close_all_files(struct list* files);

#endif /* userprog/syscall.h */
