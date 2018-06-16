#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void * is_valid_addr(const void*);
int exec_proc(char *file_name);
void exit_proc(int status);
void * is_valid_addr(const void *vaddr);
struct proc_file* search_fd(struct list* files, int fd);
void clean_single_file(struct list* files, int fd);
// void clean_all_files(struct list* files); // declear in syscall.h


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

// extern bool running;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  	int *p = f->esp;
	is_valid_addr(p);

  	int system_call = *p;
	switch (system_call)
	{
		case SYS_HALT:
		shutdown_power_off();
		break;

		case SYS_EXIT:
		syscall_exit(f);
		break;

		case SYS_EXEC:
		f->eax = syscall_exec(f);
		break;

		case SYS_WAIT:
		f->eax = syscall_wait(f);
		break;

		case SYS_CREATE:
		f->eax = syscall_creat(f);
		break;

		case SYS_REMOVE:
		f->eax = syscall_remove(f);
		break;

		case SYS_OPEN:
		f->eax = syscall_open(f);
		break;

		case SYS_FILESIZE:
		f->eax = syscall_filesize(f);
		break;

		case SYS_READ:
		f->eax = syscall_read(f);
		break;

		case SYS_WRITE:
		// 新方法会导致多fail一个multi，先使用新方法
		// 看看有什么问题

		// is_valid_addr(p+7);
		// is_valid_addr(*(p+6));
		// if(*(p+5)==1)
		// {
		// 	putbuf(*(p+6),*(p+7));
		// 	f->eax = *(p+7);
		// }
		// else
		// {
		// 	struct proc_file* fptr = search_fd(&thread_current()->files, *(p+5));
		// 	if(fptr==NULL)
		// 		f->eax = -1;
		// 	else
		// 	{
		// 		lock_acquire(&filesys_lock);
		// 		f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
		// 		lock_release(&filesys_lock);
		// 	}
		// }
		f->eax = syscall_write(f);
		break;

		case SYS_SEEK:
		syscall_seek(f);
		break;

		case SYS_TELL:
		f->eax = syscall_tell(f);
		break;

		case SYS_CLOSE:
		syscall_close(f);
		break;

		default:
		printf("Default %d\n",*p);
	}
}

int
exec_proc(char *file_name)
{
	lock_acquire(&filesys_lock);
	char * fn_cp = malloc (strlen(file_name)+1);
	  strlcpy(fn_cp, file_name, strlen(file_name)+1);
	  
	  char * save_ptr;
	  fn_cp = strtok_r(fn_cp," ",&save_ptr);

	 struct file* f = filesys_open (fn_cp);

	  if(f==NULL)
	  {
	  	lock_release(&filesys_lock);
	  	return -1;
	  }
	  else
	  {
	  	file_close(f);
	  	lock_release(&filesys_lock);
	  	return process_execute(file_name);
	  }
}

void
exit_proc(int status)
{
	//printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);
	struct list_elem *e;
	struct child_process *f;

      for (e = list_begin (&thread_current()->parent->children_list); e != list_end (&thread_current()->parent->children_list);
           e = list_next (e))
        {
          f = list_entry (e, struct child_process, child_elem);
          if(f->tid == thread_current()->tid)
          {
          	f->if_waited = true;
          	f->exit_status = status;
          }
        }
	thread_current()->exit_status = status;

	if(thread_current()->parent->waiting_child == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);

	thread_exit();
}

void *
is_valid_addr(const void *vaddr)
{
	void *page_ptr = NULL;
	if (!is_user_vaddr(vaddr) || !(page_ptr = pagedir_get_page(thread_current()->pagedir, vaddr)))
	{
		exit_proc(-1);
		return 0;
	}
	return page_ptr;
}

  /* Find fd and return process fild struct in the list,
  if not exist return NULL. */
struct proc_file *
search_fd(struct list* files, int fd)
{
	struct list_elem *e;
	struct proc_file *proc_f;
      for (e = list_begin (files); e != list_end (files); e = list_next (e))
        {
          proc_f = list_entry (e, struct proc_file, elem);
          if(proc_f->fd == fd)
          	return proc_f;
        }
   return NULL;
}

  /* close and free specific process files
  by the given fd in the file list. Firstly,
  find fd in the list, then remove it. */
void
clean_single_file(struct list* files, int fd)
{
	struct proc_file *proc_f = search_fd(files,fd);
	if (proc_f != NULL){
		file_close(proc_f->ptr);
		list_remove(&proc_f->elem);
    	free(proc_f);
	}
}

  /* close and free all process files in the file list */
void
clean_all_files(struct list* files)
{
	struct proc_file *proc_f;
	while(!list_empty(files))
	{
		proc_f = list_entry (list_pop_front(files), struct proc_file, elem);
		file_close(proc_f->ptr);
		list_remove(&proc_f->elem);
		free(proc_f);
	}
}

void
syscall_exit(struct intr_frame *f)
{
	int *p = f->esp;
	is_valid_addr(p+1);
	exit_proc(*(p+1));
}

int
syscall_exec(struct intr_frame *f)
{
	int *p = f->esp;
	is_valid_addr(p+1);
	is_valid_addr(*(p+1));
	return exec_proc(*(p+1));
}

int
syscall_wait(struct intr_frame *f)
{
	int *p = f->esp;
	is_valid_addr(p+1);
	return process_wait(*(p+1));
}

int
syscall_creat(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+5);
	is_valid_addr(*(p+4));
	lock_acquire(&filesys_lock);
	ret = filesys_create(*(p+4),*(p+5));
	lock_release(&filesys_lock);

	return ret;
}

int
syscall_remove(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+1);
	is_valid_addr(*(p+1));
	lock_acquire(&filesys_lock);
	if(filesys_remove(*(p+1))==NULL)
		ret = false;
	else
		ret = true;
	lock_release(&filesys_lock);

	return ret;
}

int
syscall_open(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+1);
	is_valid_addr(*(p+1));

	lock_acquire(&filesys_lock);
	struct file* fptr = filesys_open (*(p+1));
	lock_release(&filesys_lock);
	if(fptr==NULL)
		ret = -1;
	else
	{
		struct proc_file *pfile = malloc(sizeof(*pfile));
		pfile->ptr = fptr;
		pfile->fd = thread_current()->fd_count;
		thread_current()->fd_count++;
		list_push_back (&thread_current()->opened_files, &pfile->elem);
		ret = pfile->fd;
	}

	return ret;
}

int
syscall_filesize(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+1);
	lock_acquire(&filesys_lock);
	ret = file_length (search_fd(&thread_current()->opened_files, *(p+1))->ptr);
	lock_release(&filesys_lock);

	return ret;
}

int
syscall_read(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+7);
	is_valid_addr(*(p+6));
	if(*(p+5)==0)
	{
		int i;
		uint8_t* buffer = *(p+6);
		for(i=0;i<*(p+7);i++)
			buffer[i] = input_getc();
		ret = *(p+7);
	}
	else
	{
		struct proc_file* fptr = search_fd(&thread_current()->opened_files, *(p+5));
		if(fptr==NULL)
			ret=-1;
		else
		{
			lock_acquire(&filesys_lock);
			ret = file_read (fptr->ptr, *(p+6), *(p+7));
			lock_release(&filesys_lock);
		}
	}
		
	return ret;
}

int
syscall_write(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+7);
	is_valid_addr(*(p+6));
	if(*(p+5)==1)
	{
		putbuf(*(p+6),*(p+7));
		ret = *(p+7);
	}
	else
	{
		struct proc_file* fptr = search_fd(&thread_current()->opened_files, *(p+5));
		if(fptr==NULL)
			ret=-1;
		else
		{
			lock_acquire(&filesys_lock);
			ret = file_write (fptr->ptr, *(p+6), *(p+7));
			lock_release(&filesys_lock);
		}
	}
		
	return ret;
}

void
syscall_seek(struct intr_frame *f)
{
	int *p = f->esp;

	is_valid_addr(p+5);
	lock_acquire(&filesys_lock);
	file_seek(search_fd(&thread_current()->opened_files, *(p+4))->ptr,*(p+5));
	lock_release(&filesys_lock);
}

int
syscall_tell(struct intr_frame *f)
{
	int *p = f->esp;
	int ret;

	is_valid_addr(p+1);
	lock_acquire(&filesys_lock);
	ret = file_tell(search_fd(&thread_current()->opened_files, *(p+1))->ptr);
	lock_release(&filesys_lock);

	return ret;
}

void
syscall_close(struct intr_frame *f)
{
	int *p = f->esp;

	is_valid_addr(p+1);
	lock_acquire(&filesys_lock);
	clean_single_file(&thread_current()->opened_files,*(p+1));
	lock_release(&filesys_lock);
}